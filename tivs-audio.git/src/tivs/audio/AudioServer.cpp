#include <cetty/config/ConfigCenter.h>
#include <cetty/craft/builder/CraftServerBuilder.h>
#include <tivs/audio/AudioServiceImpl.h>

using namespace cetty::config;
using namespace cetty::craft::builder;

int main(int argc, char* argv[]) {
    ConfigCenter::instance().load(argc, argv);

    CraftServerBuilder()
        .registerService(new tivs::audio::AudioServiceImpl)
        .buildAll()
        .waitingForExit();

    return 0;
}
