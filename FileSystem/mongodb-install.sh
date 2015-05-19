echo "updating and intalling latest mongodb"
sudo apt-get update
sudo apt-get install -y --force-yes mongodb

echo "staring mongodb"
sudo /usr/bin/mongod --config /etc/mongodb.conf


tail -2 /var/log/mongodb/mongodb.log 

# curl http://localhost:27017/
