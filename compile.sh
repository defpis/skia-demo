if [ -d out ]; then
  rm -rf out
fi

mkdir out
cd out

source ../_download/skia/third_party/externals/emsdk/emsdk_env.sh

emcmake cmake .. -G Ninja

ninja -j 6

cd ..