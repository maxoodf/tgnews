# **TGNEWS: Data Clustering Contest**
The task decision of Data Clustering Context described at https://contest.com/docs/data_clustering2

## Description
`tgnews` utility data flow and used algorithms are the following:
  - language detection: Google's GLDv3 used to detect laguages (https://github.com/google/cld3.git);
  - normalization: de-facto standard UTF library (ICU) to convert documents to lowercase and remove non-alpha chars; tokenizer from https://github.com/maxoodf/word2vec;
  - documents vectorisation: all words of a document are embedded into vector space (https://github.com/maxoodf/word2vec) and each document itself is embedded into a vector with the same size as the word2vec vectors;
  - news detection: based on a very simple DNN with the loss binary layer, fully connected layer and document's vector (https://github.com/davisking/dlib);
  - category detection: based on a very simple DNN with the loss multi-class layer, fully connected layer and document's vector (https://github.com/davisking/dlib);
  - clustering: DBSCAN-based algorithm with a dynamic similarity threshold and improved neighbors detection logic;
  - clusters ordering: based on a very simple DNN with the mean squared loss layer, fully connected layer and document's vector (https://github.com/davisking/dlib).
   
## Building
Base packages installation:    
```bash
sudo apt update
sudo apt upgrade
sudo apt install -y libtool g++ git cmake pkg-config libprotobuf-dev libprotoc-dev protobuf-compiler libblas-dev liblapack-dev libicu-dev libssl-dev
sudo /usr/sbin/ldconfig
```
Google's gumbo-parser library (HTML5 parser):    
```bash
git clone https://github.com/google/gumbo-parser.git
cd ./gumbo-parser
./autogen.sh
./configure
make -j 8
sudo make install
cd ../
```
Google's CLD3 library (language detection):    
```bash
git clone https://github.com/google/cld3.git
cd ./cld3
sed -i 's/add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)//g' ./CMakeLists.txt
mkdir build-release
cd ./build-release
cmake -DCMAKE_BUILD_TYPE=Release ../
make -j 8
sudo cp ./libcld3.a /usr/local/lib
sudo mkdir /usr/local/include/google
sudo mkdir /usr/local/include/google/cld_3
sudo cp -r ./cld_3/protos /usr/local/include/google/cld_3
sudo cp -r ../src/script_span /usr/local/include/google/cld_3
sudo cp ../src/*.h /usr/local/include/google/cld_3
cd ../../
```
DLib library (DNN, clustering, etc):    
```bash
git clone https://github.com/davisking/dlib.git
cd ./dlib
git checkout tags/v19.19
mkdir ./build-release
cd ./build-release
cmake -DCMAKE_BUILD_TYPE=Release ../
make -j 8
sudo make install
cd ../../
```
Libevent library (HTTP/HTTPS server):
```bash
git clone https://github.com/libevent/libevent.git
cd libevent
git checkout tags/release-2.1.11-stable
mkdir build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-fPIC -DEVENT__LIBRARY_TYPE=STATIC -DEVENT__DISABLE_BENCHMARK:BOOL=ON -DEVENT__DISABLE_DEBUG_MODE:BOOL=ON -DEVENT__DISABLE_SAMPLES:BOOL=ON -DEVENT__DISABLE_TESTS:BOOL=ON ../
make -j 8
sudo make install
cd ../../
```
Rapidjson library (JSON parsing/writing):
```bash
git clone https://github.com/Tencent/rapidjson.git
cd rapidjson
mkdir build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ../
make -j 8
sudo make install
cd ../../
```
Sqlite3 (document attributes storage)
```bash
wget https://www.sqlite.org/2020/sqlite-autoconf-3310100.tar.gz
tar xfz ./sqlite-autoconf-3310100.tar.gz
cd ./sqlite-autoconf-3310100
./configure --enable-shared=no
make -j 8
sudo make install
cd ../
```
Word2vec++ library (words and documents embedding into vector space)
```bash
git clone https://github.com/maxoodf/word2vec.git
cd ./word2vec
mkdir ./build-release
cd ./build-release
cmake -DCMAKE_BUILD_TYPE=Release ../
make -j 8
sudo make install
cd ../../ 
```
TGNEWS utility:    
```bash
sudo /usr/sbin/ldconfig
cd ./submission/src/tgnews/
mkdir ./build-release
cd ./build-release
cmake -DCMAKE_BUILD_TYPE=Release ../
make -j 8
cd ../bin
```
MODELS:
- download model files [archive](https://drive.google.com/file/d/1CoN_59XyNdgy_Cia_bqv9LrEjMFaYhbB/view?usp=sharing) (1.2GB)
- extract files to `./model` folder
- go to `./bin` folder and run `./tgnews` for more information

#dataclustering 
Bossy Gnu's source code is available here: https://github.com/maxoodf/tgnews