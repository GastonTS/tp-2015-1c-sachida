echo "Installing libraries.."
sudo apt-get update
sudo apt-get install -y --force-yes gcc automake autoconf libtool


# Get the driver
#wget https://github.com/mongodb/mongo-c-driver/releases/download/1.1.5/mongo-c-driver-1.1.5.tar.gz
#tar xzf mongo-c-driver-1.1.5.tar.gz
#cd mongo-c-driver-1.1.5

wget https://github.com/mongodb/mongo-c-driver/releases/download/1.1.8/mongo-c-driver-1.1.8.tar.gz
tar xzf mongo-c-driver-1.1.8.tar.gz
cd mongo-c-driver-1.1.8
./configure
make
sudo make install

# Delete the driver
cd ..
rm -rf mongo-c-driver-1.1.8
rm mongo-c-driver-1.1.8.tar.gz

# Add the library 
echo "include /usr/local/lib" | sudo tee -a /etc/ld.so.conf
sudo ldconfig




# TO INSTALL THE LATEST VERSION:
#git clone git://github.com/mongodb/mongo-c-driver.git
#cd mongo-c-driver
#./autogen.sh --prefix=/usr --libdir=/usr/lib64
#make
#sudo make install




# TESTING: (blocked archive.ubuntu.com)
#sudo sed -i 's/ar.archive.ubuntu.com/ftp.tecnoera.com/g'  /etc/apt/sources.list
