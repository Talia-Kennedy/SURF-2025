#include "Gamma/Oscillator.h"
#include "Gamma/SamplePlayer.h"
#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"
#include "al/sound/al_Reverb.hpp"
#include "al/ui/al_Parameter.hpp"

using namespace al;

std::unordered_map<char, std::unordered_map<char, const char*>> filename = {
    {'1',
     {
         {'a', "Bird Chirp (long).mp3.wav"},    //
         {'k', "Bug buzzy.mp3.wav"},            //
         {'s', "Cleaner chirp.mp3.wav"},        //
         {'v', "Creepy plane.mp3.wav"},         //
         {'n', "Crunchy Walk.mp3.wav"},         //
         {'z', "E plane.mp3.wav"},              //
         {'x', "F plane.mp3.wav"},              //
         {'c', "F# plane.mp3.wav"},             //
         {'l', "F# wind.mp3.wav"},              //
         {'b', "Long walk.mp3.wav"},            //
         {'d', "Lower pitched chirp.mp3.wav"},  //
         {'f', "Person _hold on_.mp3.wav"},     //
         {'g', "Person _ya_.mp3.wav"},          //
         {'j', "Rumbling wind.mp3.wav"},        //
         {'h', "Soft wind.mp3.wav"},            //
     }},
    {'2',
     {
         {'w', "Another wave.mp3.wav"},      //
         {'p', "Birds (multiple).mp3.wav"},  //
         {'a', "Birds loud.mp3.wav"},        //
         {'d', "Buzz (Bee_).mp3.wav"},       //
         {'i', "Cling Clank.mp3.wav"},       //
         {'s', "Cycling wind.mp3.wav"},      //
         {'g', "F plane.mp3.wav"},           //
         {'j', "F# Plane.mp3.wav"},          //
         {'l', "Footsteps 1.mp3.wav"},       //
         {';', "Footsteps 2.mp3.wav"},       //
         {'t', "Kid upset_.mp3.wav"},        //
         {'q', "Ocean Spray.mp3.wav"},       //
         {'y', "People (oh no).mp3.wav"},    //
         {'t', "People (woo).mp3.wav"},      //
         {'o', "Percussive click.mp3.wav"},  //
         {'f', "Plane E (Loud).mp3.wav"},    //
         {'h', "Plane F (loud).mp3.wav"},    //
         {'k', "Plane F# vib.mp3.wav"},      //
         {'r', "Wave Dramatic.mp3.wav"},     //
         {'e', "Wave choppy.mp3.wav"},       //
     }},
    {'3',
     {
         {'f', "B plane.mp3.wav"},                             //
         {'e', "Bike (Crank).mp3.wav"},                        //
         {'r', "Bike (long).mp3.wav"},                         //
         {'t', "Bike (quiet to loud).mp3.wav"},                //
         {'y', "Bike (sudden rush).mp3.wav"},                  //
         {'q', "Bike 6.mp3.wav"},                              //
         {'w', "Bike 8.mp3.wav"},                              //
         {'u', "Bike Creaky tires.mp3.wav"},                   //
         {'i', "Bike click.mp3.wav"},                          //
         {'l', "Bikepath walking clip.mp3.wav"},               //
         {'g', "C plane.mp3.wav"},                             //
         {'s', "More birds.mp3.wav"},                          //
         {'p', "People (he lives a different life).mp3.wav"},  //
         {'o', "Person (im indifferent).mp3.wav"},             //
         {'h', "Plane C# w bike.mp3.wav"},                     //
         {'k', "Plane F w bike.mp3.wav"},                      //
         {'d', "Plane humm.mp3.wav"},                          //
         {'j', "Rumble D plane.mp3.wav"},                      //
         {'a', "birds.mp3.wav"},                               //
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

  void onProcess(Graphics& g) override {
    g.draw(mesh);
  }

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
};

int main() {
  MyApp app;
  app.configureAudio(48000, 512, 2, 0);
  gam::sampleRate(48000);
  app.start();
  return 0;
}