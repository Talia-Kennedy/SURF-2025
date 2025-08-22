#include "Gamma/Oscillator.h"
#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"
#include "al/sound/al_Reverb.hpp"
#include "al/ui/al_Parameter.hpp"

using namespace al;

inline float mtof(float m) { return 8.175799f * powf(2.0f, m / 12.0f); }

class Bonk : public SynthVoice {
  gam::SineD<> bonk;
  Mesh mesh;  // The mesh now belongs to the voice
  int frame = 0;

 public:
  Bonk() { mesh.primitive(Mesh::TRIANGLES); }

  void onProcess(AudioIOData& io) override {
    while (io()) {
      io.out(0) += bonk();
    }
  }

  void onProcess(Graphics& g) override {
    frame++;
    if (frame == 60) {
      free();
    }
  }

  void set() {
    bonk.freq(mtof(rnd::uniform(127.0f)));
    bonk.decay(1.0);
    bonk.reset();
  }
};

struct MyApp : App {
  Parameter bandwidth{"bandwidth", 0.9995, 0.8, 0.9999999};
  Parameter decay{"decay", 0.85, 0.3, 0.997};
  Parameter damping{"damping", 0.4, 0.1, 0.9};
  Parameter wetness{"wetness", 0.1, 0.0, 1.0};

  PresetHandler presetHandler{"sequencer_preset"};
  PolySynth synth;
  Reverb<> reverb;

  void onInit() override {
    synth.allocatePolyphony<Bonk>(16);
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto& gui = GUIdomain->newGUI();
    gui.add(presetHandler);
    gui << wetness << bandwidth << decay << damping;
    presetHandler << wetness << bandwidth << decay << damping;
    parameterServer() << wetness << bandwidth << decay << damping;
  }

  void onDraw(Graphics& g) override {
    g.clear(0.3);
    synth.render(g);
  }

  void onSound(AudioIOData& io) override {
    synth.render(io);
    reverb.bandwidth(bandwidth).decay(decay).damping(damping);
    while (io()) {
      reverb.mix(io.out(0), io.out(1), wetness);
      //reverb(io.out(0) + io.out(1), io.out(0), io.out(1));
    }
  }

  bool onKeyDown(const Keyboard& k) override {
    auto* voice = synth.getVoice<Bonk>();
    voice->set();
    synth.triggerOn(voice);
    return true;
  }
};

int main() {
  MyApp app;
  app.configureAudio(48000, 512, 2, 0);
  gam::sampleRate(48000);
  app.start();
  return 0;
}