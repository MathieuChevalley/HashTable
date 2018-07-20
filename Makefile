CFLAGS = -Wall -g -DDEBUG -std=c99
LDLIBS = -lcheck -lm -lrt -pthread -lcrypto

all: test-hashtable pps-launch-server pps-client-put pps-client-get pps-list-nodes pps-dump-node pps-client-cat pps-client-substr pps-client-find
	@echo "Création des exécutables"

network.o: network.c network.h
client.o: client.c client.h config.h system.h
node.o: node.c node.h system.h
node_list.o: node_list.c node_list.h ring.h
system.o: system.c system.h error.h
hashtable.o: hashtable.c hashtable.h error.h util.h
args.o: args.c args.h error.h
util.o: util.c util.h
ring.o: ring.c ring.h

error.o: error.c error.h
test-hashtable.o: test-hashtable.c tests.h hashtable.h error.h
pps-launch-server.o: pps-launch-server.c hashtable.h system.h config.h
pps-client-put.o: pps-client-put.c network.h
pps-client-get.o: pps-client-get.c network.h

pps-list-nodes.o: pps-list-nodes.c config.h error.h system.h node_list.h ring.h
pps-dump-node.o: pps-dump-node.c config.h error.h system.h
pps-client-cat.o: pps-client-cat.c network.h config.h
pps-client-substr.o: pps-client-substr.c network.h 
pps-client-find.o: pps-client-find.c network.h

test-hashtable: test-hashtable.o hashtable.o error.o 
pps-launch-server: pps-launch-server.o system.o hashtable.o error.o
pps-client-put: pps-client-put.o network.o client.o ring.o hashtable.o node.o node_list.o system.o error.o args.o
pps-client-get: pps-client-get.o network.o client.o ring.o hashtable.o node.o node_list.o system.o error.o args.o
pps-list-nodes: pps-list-nodes.o error.o system.o node.o node_list.o ring.o
pps-dump-node: pps-dump-node.o error.o system.o
pps-client-cat: pps-client-cat.o network.o client.o ring.o hashtable.o node.o node_list.o system.o error.o args.o util.o
pps-client-substr: pps-client-substr.o network.o client.o ring.o hashtable.o node.o node_list.o system.o error.o args.o
pps-client-find: pps-client-find.o network.o client.o ring.o hashtable.o node.o node_list.o system.o error.o args.o

