################################## Makefile ###################################
# Author: sd001 > Bruno Neves, n.º 31614
#               > Vasco Orey,  n.º 32550
###############################################################################

build: table-client table-server

############################## table-client ##############################

table-client: client-lib.o table-client.o
	gcc -lm -lpthread client-lib.o table-client.o -o table-client

client-lib.o: data.o entry.o list.o table.o base64.o message.o remote_table.o network_client.o quorum_table.o quorum_access.o
	ld -r data.o entry.o list.o table.o base64.o message.o remote_table.o network_client.o quorum_table.o quorum_access.o -o client-lib.o

table-client.o: table_client.c utils.h
	gcc -g -c -Wall table_client.c -o table-client.o

###############################################################################



############################## table-server ##############################

table-server: data.o entry.o list.o table.o base64.o message.o table_skel.o table_server.o persistent_table.o persistence_manager.o
	gcc -lm data.o entry.o list.o table.o base64.o message.o table_skel.o table_server.o persistent_table.o persistence_manager.o -o table-server

table-server.o: table-server.c utils.h
	gcc -g -c -Wall table-server.c

###############################################################################



################################# Biblioteca ##################################
data.o: data.c data.h utils.h
	gcc -g -c -Wall data.c

entry.o: entry.c entry.h utils.h
	gcc -g -c -Wall entry.c

list.o: list.c list.h list-private.h utils.h
	gcc -g -c -Wall list.c

table.o: table.c table.h table-private.h utils.h
	gcc -g -c -Wall table.c

base64.o: base64.c base64.h
	gcc -g -c -Wall base64.c
	
message.o: message.c message.h message-private.h utils.h
	gcc -g -c -Wall message.c

remote_table.o: remote_table.c remote_table.h remote_table-private.h utils.h
	gcc -g -c -Wall remote_table.c

network_client.o: network_client.c network_client.h utils.h
	gcc -g -c -Wall network_client.c

table_skel.o: table_skel.c table_skel.h utils.h
	gcc -g -c -Wall table_skel.c

persistent_table.o: persistent_table.c persistent_table.h persistent_table-private.h
	gcc -g -c -Wall persistent_table.c

persistence_manager.o: persistence_manager.c persistence_manager.h persistence_manager-private.h
	gcc -g -c -Wall persistence_manager.c

quorum_table.o: quorum_table.c quorum_table.h quorum_table-private.h
	gcc -g -c -Wall quorum_table.c

quorum_access.o: quorum_access.c quorum_access.h quorum_access-private.h
	gcc -g -c -Wall -lpthread quorum_access.c

###############################################################################



#################################### clean ####################################

clean:
	rm -rf *.o table-client table-server

###############################################################################
