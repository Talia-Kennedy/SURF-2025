#include "Gamma/Oscillator.h"
#include "Gamma/SamplePlayer.h"
#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"
#include "al/sound/al_Reverb.hpp"
#include "al/ui/al_Parameter.hpp"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

using namespace al;

class AudioRecording : std::vector<float> {
 public:
  void operator()(float t) { push_back(t); }
  void save(const char* filename) {
    // TBD: implement saving

    drwav_data_format format;
    format.channels = 1;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.sampleRate = 48000;
    format.bitsPerSample = 32;
    drwav* pWav = drwav_open_file_write(filename, &format);
    if (pWav == nullptr) {
      exit(1);
    }
    for (auto f : *this) {
      if (1 != drwav_write(pWav, 1, &f)) {
        exit(1);
      }
    }
    drwav_close(pWav);
  }
};

std::unordered_map<char, std::unordered_map<char, const char*>>
    filename = {{'1',
                 {
                     {'a', "Bird Chirp (long).wav"},    //
                     {'k', "Bug buzzy.wav"},            //
                     {'s', "Cleaner chirp.wav"},        //
                     {'v', "Creepy plane.wav"},         //
                     {'n', "Crunchy Walk.wav"},         //
                     {'z', "E plane.wav"},              //
                     {'x', "F plane.wav"},              //
                     {'c', "F# plane.wav"},             //
                     {'l', "F# wind.wav"},              //
                     {'b', "Long walk.wav"},            //
                     {'d', "Lower pitched chirp.wav"},  //
                     {'f', "Person _hold on_.wav"},     //
                     {'g', "Person _ya_.wav"},          //
                     {'j', "Rumbling wind.wav"},        //
                     {'h', "Soft wind.wav"},            //
                 }},
                {'2',
                 {
                     {'w', "Another wave.wav"},      //
                     {'p', "Birds (multiple).wav"},  //
                     {'a', "Birds loud.wav"},        //
                     {'d', "Buzz (Bee?).wav"},       //
                     {'i', "Cling Clank.wav"},       //
                     {'s', "Cycling wind.wav"},      //
                     {'g', "F plane.wav"},           //
                     {'j', "F# Plane.wav"},          //
                     {'l', "Footsteps 1.wav"},       //
                     {';', "Footsteps 2.wav"},       //
                     {'t', "Kid upset?.wav"},        //
                     {'q', "Ocean Spray.wav"},       //
                     {'y', "People (oh no).wav"},    //
                     {'u', "People (woo).wav"},      //
                     {'o', "Percussive click.wav"},  //
                     {'f', "Plane E (Loud).wav"},    //
                     {'h', "Plane F (loud).wav"},    //
                     {'k', "Plane F# vib.wav"},      //
                     {'r', "Wave Dramatic.wav"},     //
                     {'e', "Wave choppy.wav"},       //
                 }},
                {'3',
                 {
                     {'f', "B plane.wav"},                             //
                     {'e', "Bike (Crank).wav"},                        //
                     {'r', "Bike (long).wav"},                         //
                     {'t', "Bike (quiet to loud).wav"},                //
                     {'y', "Bike (sudden rush).wav"},                  //
                     {'q', "Bike 6.wav"},                              //
                     {'w', "Bike 8.wav"},                              //
                     {'u', "Bike Creaky tires.wav"},                   //
                     {'i', "Bike click.wav"},                          //
                     {'l', "Bikepath walking clip.wav"},               //
                     {'g', "C plane.wav"},                             //
                     {'s', "More birds.wav"},                          //
                     {'p', "People (he lives a different life).wav"},  //
                     {'o', "Person (im indifferent).wav"},             //
                     {'h', "Plane C# w bike.wav"},                     //
                     {'k', "Plane F w bike.wav"},                      //
                     {'d', "Plane humm.wav"},                          //
                     {'j', "Rumble D plane.wav"},                      //
                     {'a', "birds.wav"},                               //
                 }}};

std::unordered_map<
    char, std::unordered_map<char, std::unique_ptr<gam::SamplePlayer<float>>>>
    clip;

inline float mtof(float m) { return 8.175799f * powf(2.0f, m / 12.0f); }

class Clip : public SynthVoice {
  gam::SamplePlayer<float> player;
  Mesh mesh;

 public:
  Clip() {
    mesh.primitive(Mesh::TRIANGLES);

    for (int i = 0; i < 3; i++) {
      mesh.vertex(rnd::ball<Vec3f>());
      mesh.color(rnd::uniform(1.0), rnd::uniform(1.0), rnd::uniform(1.0));
    }
  }

  void onProcess(AudioIOData& io) override {
    while (io()) {
      io.out(0) += player();
    }
    if (player.done()) {
      free();
    }
  }

  void onProcess(Graphics& g) override { g.draw(mesh); }

  void set(gam::SamplePlayer<float>& p, float rate = 1.0f) {
    player.buffer(p);
    player.rate(rate);
    player.pos(0);
    player.max(player.size());
    player.min(0);
    player.reset();
  }
};

struct MyApp : App {
  Parameter bandwidth{"bandwidth", 0.9995, 0.8, 0.9999999};
  Parameter decay{"decay", 0.85, 0.3, 0.997};
  Parameter damping{"damping", 0.4, 0.1, 0.9};
  Parameter wetness{"wetness", 0.1, 0.0, 1.0};
  Parameter rate{"rate", 1.0, 0.5, 2.0};

  PresetHandler presetHandler{"sequencer_preset"};
  PolySynth synth;
  Reverb<> reverb;
  AudioRecording recording;

  void onInit() override {
    synth.allocatePolyphony<Clip>(16);
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto& gui = GUIdomain->newGUI();
    gui.add(presetHandler);
    gui << wetness << bandwidth << decay << damping << rate;
    presetHandler << wetness << bandwidth << decay << damping << rate;
    parameterServer() << wetness << bandwidth << decay << damping << rate;

    // load all the samples...
    for (auto& bank : filename) {
      for (auto& binding : bank.second) {
        printf("Loading bank %c key %c file %s\n", bank.first, binding.first,
               binding.second);
        clip[bank.first][binding.first].reset(
            new gam::SamplePlayer<float>(binding.second));
      }
    }
  }

  void onCreate() override {
    nav().pos(0, 0, 4);
    lens().fovy(60);
    navControl().disable();
  }

  void onDraw(Graphics& g) override {
    g.clear(0.2);
    g.meshColor();
    synth.render(g);
  }

  void onSound(AudioIOData& io) override {
    synth.render(io);
    reverb.bandwidth(bandwidth).decay(decay).damping(damping);
    while (io()) {
      reverb.mix(io.out(0), io.out(1), wetness);
      // reverb(io.out(0) + io.out(1), io.out(0), io.out(1));
      recording(io.out(0));
    }
  }
  char which = '1';
  bool onKeyDown(const Keyboard& k) override {
    if (k.key() >= '1' && k.key() <= '3') {
      which = k.key();
      std::cout << "Switched to bank " << which << std::endl;
      return true;
    }
    auto& bank = clip[which];
    if (bank.count(k.key()) == 0) {
      std::cout << "No sample for key " << k.key() << " in bank " << which
                << std::endl;
      return true;
    }

    auto* voice = synth.getVoice<Clip>();
    voice->set(*clip[which][k.key()], rate);
    synth.triggerOn(voice);
    return true;
  }

  void onExit() override {
    char filename[32];
    snprintf(filename, sizeof(filename), "recording-%d.wav", rnd::uniform(10000));  
    recording.save(filename);
  } 
};

int main() {
  MyApp app;
//  AudioDevice device = AudioDevice("ScreenRecording");
//  device.print();
//  app.configureAudio(device, 48000, 512, 2, 0);
  app.configureAudio(48000, 512, 2, 0);
  gam::sampleRate(48000);
  app.start();
  return 0;
}