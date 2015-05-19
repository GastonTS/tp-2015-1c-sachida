#echo "adding key"
#sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 7F0CEB10

#echo "adding source"
#echo "deb http://repo.mongodb.org/apt/ubuntu "$(lsb_release -sc)"/mongodb-org/3.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-3.0.list

echo "updating and intalling latest mongodb"
sudo apt-get update
#sudo apt-get install -y mongodb-org
sudo apt-get install -y --force-yes mongodb

#pin versions
#echo "pin versions.."
#echo "mongodb-org hold" | sudo dpkg --set-selections
#echo "mongodb-org-server hold" | sudo dpkg --set-selections
#echo "mongodb-org-shell hold" | sudo dpkg --set-selections
#echo "mongodb-org-mongos hold" | sudo dpkg --set-selections
#echo "mongodb-org-tools hold" | sudo dpkg --set-selections

#start mongodb..
echo "staring mongodb"
#sudo service mongod start
sudo /usr/bin/mongod --config /etc/mongodb.conf

#echo "The following line should say something like: '[initandlisten] waiting for connections on port 27017'"
#tail -1 /var/log/mongodb/mongod.log

# curl http://localhost:27017/
