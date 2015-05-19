echo "Install libraries.."
sudo apt-get update
sudo apt-get install -y --force-yes gcc automake autoconf libtool

wget https://github.com/mongodb/mongo-c-driver/releases/download/1.1.5/mongo-c-driver-1.1.5.tar.gz
tar xzf mongo-c-driver-1.1.5.tar.gz
cd mongo-c-driver-1.1.5
./configure
make
sudo make install

echo "include /usr/local/lib" | sudo tee -a /etc/ld.so.conf

#autogen from git
#git clone git://github.com/mongodb/mongo-c-driver.git
#cd mongo-c-driver
#./autogen.sh --prefix=/usr --libdir=/usr/lib64
#make
#sudo make install
