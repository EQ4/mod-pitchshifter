#include <stdlib.h>
#include <cmath>
#include <lv2.h>
#include "PitchShifterClasses.h"
#include "GainClass.h"

/**********************************************************************************************************************************************************/

#define PLUGIN_URI "http://moddevices.com/plugins/mod-devel/SuperCapo"
#define N_SAMPLES_DEFAULT 256
enum {IN, OUT, STEP, GAIN, PLUGIN_PORT_COUNT};

/**********************************************************************************************************************************************************/

class SuperCapo
{
public:
    SuperCapo(uint32_t n_samples, int nBuffers, double samplerate, const std::string& wfile)
    {
        wisdomFile = wfile;
        Construct(n_samples, nBuffers, samplerate, wfile.c_str());
    }
    ~SuperCapo(){Destruct();}
    void Construct(uint32_t n_samples, int nBuffers, double samplerate, const char* wisdomFile)
    {
    	this->nBuffers = nBuffers;
        SampleRate = samplerate;

        obja = new PSAnalysis(n_samples, nBuffers, wisdomFile);
        objs = new PSSinthesis(obja, wisdomFile);
        objg = new GainClass(n_samples);

        cont = 0;
    }
    void Destruct()
    {
    	delete obja;
        delete objs;
        delete objg;
    }
    void Realloc(uint32_t n_samples, int nBuffers)
    {
    	Destruct();
    	Construct(n_samples, nBuffers, SampleRate, wisdomFile.c_str());
    }

    static LV2_Handle instantiate(const LV2_Descriptor* descriptor, double samplerate, const char* bundle_path, const LV2_Feature* const* features);
    static void activate(LV2_Handle instance);
    static void deactivate(LV2_Handle instance);
    static void connect_port(LV2_Handle instance, uint32_t port, void *data);
    static void run(LV2_Handle instance, uint32_t n_samples);
    static void cleanup(LV2_Handle instance);
    static const void* extension_data(const char* uri);
    float *ports[PLUGIN_PORT_COUNT];
    
    PSAnalysis *obja;
    PSSinthesis *objs;
    GainClass *objg;

    int nBuffers;
    int cont;
    double SampleRate;
    std::string wisdomFile;
};

/**********************************************************************************************************************************************************/

static const LV2_Descriptor Descriptor = {
    PLUGIN_URI,
    SuperCapo::instantiate,
    SuperCapo::connect_port,
    SuperCapo::activate,
    SuperCapo::run,
    SuperCapo::deactivate,
    SuperCapo::cleanup,
    SuperCapo::extension_data
};

/**********************************************************************************************************************************************************/

LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    if (index == 0) return &Descriptor;
    else return NULL;
}

/**********************************************************************************************************************************************************/

LV2_Handle SuperCapo::instantiate(const LV2_Descriptor* descriptor, double samplerate, const char* bundle_path, const LV2_Feature* const* features)
{
    std::string wisdomFile = bundle_path;
    wisdomFile += "/harmonizer.wisdom";
    SuperCapo *plugin = new SuperCapo(N_SAMPLES_DEFAULT, 38, samplerate, wisdomFile);
    return (LV2_Handle)plugin;
}

/**********************************************************************************************************************************************************/

void SuperCapo::activate(LV2_Handle instance){}

/**********************************************************************************************************************************************************/

void SuperCapo::deactivate(LV2_Handle instance){}

/**********************************************************************************************************************************************************/

void SuperCapo::connect_port(LV2_Handle instance, uint32_t port, void *data)
{
    SuperCapo *plugin;
    plugin = (SuperCapo *) instance;
    plugin->ports[port] = (float*) data;
}

/**********************************************************************************************************************************************************/

void SuperCapo::run(LV2_Handle instance, uint32_t n_samples)
{
    SuperCapo *plugin;
    plugin = (SuperCapo *) instance;

    float *in   = plugin->ports[IN];
    float *out  = plugin->ports[OUT];
    double s    = (double)(*(plugin->ports[STEP]));
    double gain = (double)(*(plugin->ports[GAIN]));
    
    if ( (plugin->obja)->hopa != (int)n_samples )
    {
        plugin->Realloc(n_samples, nBuffersSW(n_samples,38,20,12,8));
		return;
	}

    if (InputAbsSum(in, n_samples) == 0)
    {
        fill_n(out,n_samples,0);
        return;
    }

    (plugin->objg)->SetGaindB(gain);
    (plugin->obja)->PreAnalysis(plugin->nBuffers, in);
    (plugin->objs)->PreSinthesis();
        
    if (plugin->cont < plugin->nBuffers-1)
        plugin->cont = plugin->cont + 1;
    else
    {
        (plugin->obja)->Analysis();
        (plugin->objs)->Sinthesis(s);
        (plugin->objg)->SimpleGain((plugin->objs)->yshift, out);
    }
}

/**********************************************************************************************************************************************************/

void SuperCapo::cleanup(LV2_Handle instance)
{
    delete ((SuperCapo *) instance);
}

/**********************************************************************************************************************************************************/

const void* SuperCapo::extension_data(const char* uri)
{
    return NULL;
}