// Stereo gain: a minimal CLAP audio effect for AudioTap (and the AudioFly server).
// One parameter, "Gain" (dB), smoothed so changes never click. Plain C11.
// MIT licence, like everything in this folder.

#include <clap/clap.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define PLUGIN_ID "org.audiotap.example.stereo-gain"
#define PARAM_GAIN_ID 1
#define GAIN_MIN_DB (-24.0)
#define GAIN_MAX_DB 12.0
#define GAIN_DEFAULT_DB 0.0
#define SMOOTH_SECONDS 0.02 /* time constant of the smoothing */
#define STATE_MAGIC 0x53474E31u /* "SGN1" */

typedef struct {
    clap_plugin_t plugin;
    const clap_host_t *host;
    const clap_host_params_t *host_params; // may be NULL
    double sample_rate;
    // Written on the main thread (state load) or by parameter events on the audio
    // thread; read on the audio thread. Only ever touched from process() while
    // processing, and from the main thread while deactivated or from state load,
    // which CLAP says may not overlap process() on the same instance.
    double target_db;
    double current_gain; // linear, smoothed
    double smooth_coef;  // per-sample one-pole coefficient
} sg_t;

static double db_to_gain(double db) { return pow(10.0, db / 20.0); }

static double clamp_db(double db) {
    if (!(db >= GAIN_MIN_DB)) return GAIN_MIN_DB; // also catches NaN
    if (db > GAIN_MAX_DB) return GAIN_MAX_DB;
    return db;
}

// ---- audio-ports: one stereo input and one stereo output (the main ports) ----

static uint32_t ports_count(const clap_plugin_t *plugin, bool is_input) {
    (void)plugin; (void)is_input;
    return 1;
}

static bool ports_get(const clap_plugin_t *plugin, uint32_t index, bool is_input,
                      clap_audio_port_info_t *info) {
    (void)plugin;
    if (index != 0) return false;
    info->id = 0;
    snprintf(info->name, sizeof(info->name), "%s", is_input ? "Input" : "Output");
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->channel_count = 2;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID; // host may not pass the same buffer; we handle both
    return true;
}

static const clap_plugin_audio_ports_t audio_ports_ext = {ports_count, ports_get};

// ---- params ----

static uint32_t params_count(const clap_plugin_t *plugin) { (void)plugin; return 1; }

static bool params_get_info(const clap_plugin_t *plugin, uint32_t index, clap_param_info_t *info) {
    (void)plugin;
    if (index != 0) return false;
    memset(info, 0, sizeof(*info));
    info->id = PARAM_GAIN_ID;
    info->flags = CLAP_PARAM_IS_AUTOMATABLE;
    snprintf(info->name, sizeof(info->name), "%s", "Gain");
    info->module[0] = '\0';
    info->min_value = GAIN_MIN_DB;
    info->max_value = GAIN_MAX_DB;
    info->default_value = GAIN_DEFAULT_DB;
    return true;
}

static bool params_get_value(const clap_plugin_t *plugin, clap_id id, double *value) {
    const sg_t *s = (const sg_t *)plugin->plugin_data;
    if (id != PARAM_GAIN_ID) return false;
    *value = s->target_db;
    return true;
}

static bool params_value_to_text(const clap_plugin_t *plugin, clap_id id, double value, char *out,
                                 uint32_t size) {
    (void)plugin;
    if (id != PARAM_GAIN_ID) return false;
    snprintf(out, size, "%.1f dB", value);
    return true;
}

static bool params_text_to_value(const clap_plugin_t *plugin, clap_id id, const char *text,
                                 double *out) {
    (void)plugin;
    if (id != PARAM_GAIN_ID) return false;
    char *end = NULL;
    double v = strtod(text, &end);
    if (end == text) return false;
    *out = clamp_db(v);
    return true;
}

static void apply_param_event(sg_t *s, const clap_event_header_t *h) {
    if (h->space_id != CLAP_CORE_EVENT_SPACE_ID || h->type != CLAP_EVENT_PARAM_VALUE) return;
    const clap_event_param_value_t *e = (const clap_event_param_value_t *)h;
    if (e->param_id == PARAM_GAIN_ID) s->target_db = clamp_db(e->value);
}

static void params_flush(const clap_plugin_t *plugin, const clap_input_events_t *in,
                         const clap_output_events_t *out) {
    (void)out;
    sg_t *s = (sg_t *)plugin->plugin_data;
    for (uint32_t i = 0, n = in->size(in); i < n; i++) apply_param_event(s, in->get(in, i));
}

static const clap_plugin_params_t params_ext = {params_count,         params_get_info,
                                                params_get_value,     params_value_to_text,
                                                params_text_to_value, params_flush};

// ---- state: magic + the gain in dB, as a small fixed record ----

static bool state_save(const clap_plugin_t *plugin, const clap_ostream_t *stream) {
    const sg_t *s = (const sg_t *)plugin->plugin_data;
    uint32_t magic = STATE_MAGIC;
    double db = s->target_db;
    return stream->write(stream, &magic, sizeof(magic)) == (int64_t)sizeof(magic) &&
           stream->write(stream, &db, sizeof(db)) == (int64_t)sizeof(db);
}

static bool state_load(const clap_plugin_t *plugin, const clap_istream_t *stream) {
    sg_t *s = (sg_t *)plugin->plugin_data;
    uint32_t magic = 0;
    double db = 0.0;
    if (stream->read(stream, &magic, sizeof(magic)) != (int64_t)sizeof(magic) || magic != STATE_MAGIC)
        return false;
    if (stream->read(stream, &db, sizeof(db)) != (int64_t)sizeof(db)) return false;
    s->target_db = clamp_db(db);
    // Tell the host the parameter values changed, so it refreshes its controls (the host's
    // generic Gain slider, and any validator, expects this after a state load).
    if (s->host_params) s->host_params->rescan(s->host, CLAP_PARAM_RESCAN_VALUES);
    return true;
}

static const clap_plugin_state_t state_ext = {state_save, state_load};

// ---- plugin ----

static bool plugin_init(const clap_plugin_t *plugin) {
    sg_t *s = (sg_t *)plugin->plugin_data;
    s->host_params = (const clap_host_params_t *)s->host->get_extension(s->host, CLAP_EXT_PARAMS);
    return true;
}

static void plugin_destroy(const clap_plugin_t *plugin) {
    free(plugin->plugin_data);
}

static bool plugin_activate(const clap_plugin_t *plugin, double sample_rate, uint32_t min_frames,
                            uint32_t max_frames) {
    (void)min_frames; (void)max_frames;
    sg_t *s = (sg_t *)plugin->plugin_data;
    s->sample_rate = sample_rate;
    s->smooth_coef = 1.0 - exp(-1.0 / (SMOOTH_SECONDS * sample_rate));
    s->current_gain = db_to_gain(s->target_db); // start at the target: no ramp on turn-on
    return true;
}

static void plugin_deactivate(const clap_plugin_t *plugin) { (void)plugin; }
static bool plugin_start_processing(const clap_plugin_t *plugin) { (void)plugin; return true; }
static void plugin_stop_processing(const clap_plugin_t *plugin) { (void)plugin; }
static void plugin_reset(const clap_plugin_t *plugin) {
    sg_t *s = (sg_t *)plugin->plugin_data;
    s->current_gain = db_to_gain(s->target_db);
}

// Real-time safe: no allocation, locks, I/O or logging in here.
static void render(sg_t *s, const clap_process_t *p, uint32_t from, uint32_t to) {
    const clap_audio_buffer_t *in = &p->audio_inputs[0];
    clap_audio_buffer_t *out = &p->audio_outputs[0];
    const float *in_l = in->data32[0], *in_r = in->data32[1];
    float *out_l = out->data32[0], *out_r = out->data32[1];
    const double target = db_to_gain(s->target_db);
    double g = s->current_gain;
    const double k = s->smooth_coef;
    for (uint32_t i = from; i < to; i++) {
        g += (target - g) * k;
        out_l[i] = in_l[i] * (float)g; // reading before writing makes in-place safe
        out_r[i] = in_r[i] * (float)g;
    }
    s->current_gain = g;
}

static clap_process_status plugin_process(const clap_plugin_t *plugin, const clap_process_t *p) {
    sg_t *s = (sg_t *)plugin->plugin_data;
    const uint32_t frames = p->frames_count;
    const uint32_t n_events = p->in_events->size(p->in_events);
    uint32_t pos = 0;
    // Apply each parameter event at its own sample position, rendering up to it first.
    for (uint32_t e = 0; e < n_events; e++) {
        const clap_event_header_t *h = p->in_events->get(p->in_events, e);
        uint32_t t = h->time < frames ? h->time : frames;
        if (t > pos) { render(s, p, pos, t); pos = t; }
        apply_param_event(s, h);
    }
    if (pos < frames) render(s, p, pos, frames);
    return CLAP_PROCESS_CONTINUE;
}

static const void *plugin_get_extension(const clap_plugin_t *plugin, const char *id) {
    (void)plugin;
    if (!strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &audio_ports_ext;
    if (!strcmp(id, CLAP_EXT_PARAMS)) return &params_ext;
    if (!strcmp(id, CLAP_EXT_STATE)) return &state_ext;
    return NULL;
}

static void plugin_on_main_thread(const clap_plugin_t *plugin) { (void)plugin; }

static const char *const features[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_UTILITY,
                                       CLAP_PLUGIN_FEATURE_STEREO, NULL};

static const clap_plugin_descriptor_t descriptor = {
    .clap_version = CLAP_VERSION_INIT,
    .id = PLUGIN_ID,
    .name = "Stereo Gain",
    .vendor = "AudioTap examples",
    .url = "https://github.com/Shaydu/audiotap-downloads",
    .manual_url = "https://github.com/Shaydu/audiotap-downloads",
    .support_url = "https://github.com/Shaydu/audiotap-downloads",
    .version = "1.0.0",
    .description = "Turns the sound up or down. One smoothed Gain control, -24 to +12 dB.",
    .features = features,
};

static const clap_plugin_t *create_plugin(const clap_host_t *host) {
    sg_t *s = (sg_t *)calloc(1, sizeof(sg_t));
    if (!s) return NULL;
    s->host = host;
    s->target_db = GAIN_DEFAULT_DB;
    s->current_gain = db_to_gain(GAIN_DEFAULT_DB);
    s->plugin.desc = &descriptor;
    s->plugin.plugin_data = s;
    s->plugin.init = plugin_init;
    s->plugin.destroy = plugin_destroy;
    s->plugin.activate = plugin_activate;
    s->plugin.deactivate = plugin_deactivate;
    s->plugin.start_processing = plugin_start_processing;
    s->plugin.stop_processing = plugin_stop_processing;
    s->plugin.reset = plugin_reset;
    s->plugin.process = plugin_process;
    s->plugin.get_extension = plugin_get_extension;
    s->plugin.on_main_thread = plugin_on_main_thread;
    return &s->plugin;
}

// ---- factory and entry ----

static uint32_t factory_count(const clap_plugin_factory_t *f) { (void)f; return 1; }

static const clap_plugin_descriptor_t *factory_get_descriptor(const clap_plugin_factory_t *f,
                                                              uint32_t index) {
    (void)f;
    return index == 0 ? &descriptor : NULL;
}

static const clap_plugin_t *factory_create(const clap_plugin_factory_t *f, const clap_host_t *host,
                                           const char *plugin_id) {
    (void)f;
    if (!clap_version_is_compatible(host->clap_version)) return NULL;
    if (strcmp(plugin_id, PLUGIN_ID) != 0) return NULL;
    return create_plugin(host);
}

static const clap_plugin_factory_t factory = {factory_count, factory_get_descriptor, factory_create};

static bool entry_init(const char *plugin_path) { (void)plugin_path; return true; }
static void entry_deinit(void) {}
static const void *entry_get_factory(const char *factory_id) {
    return strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0 ? &factory : NULL;
}

CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, entry_init, entry_deinit,
                                                    entry_get_factory};
