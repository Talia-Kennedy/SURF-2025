#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/ui/al_Parameter.hpp"

using namespace al;

struct MyApp : App {
  Parameter mode{"Mode", 0, 0, 1};
  ParameterColor color{"Color"};
  PresetHandler presetHandler{"sequencer_preset"};

  void onInit() override {
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto& gui = GUIdomain->newGUI();
    gui.add(presetHandler);
    gui.add(mode).add(color);
    presetHandler << mode << color;
    parameterServer() << mode << color; 
  }

  void onDraw(Graphics& g) override {
    g.clear(color.get().r, color.get().g, color.get().b, color.get().a);
  }
};

int main() {
    MyApp app;
    app.configureAudio(48000, 512, 2, 0);
    app.start();
    return 0;
}