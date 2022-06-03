 #!/bin/zsh
git submodule update --init

./scripts/darwin64_build.sh
#./scripts/linux64_build.sh
#./scripts/win64_build.sh

./scripts/pack_fmu.sh