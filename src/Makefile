smredis: server.c
	$(CC) server.c -o smredis -Wall -Wextra -pedantic -ggdb -std=c99

smclient: client.c
	$(CC) client.c -o smclient  -Wall -Wextra -pedantic -ggdb -std=c99

clean:
	rm  -rf smredis smclient

show:
	firefox  https://build-your-own.org/redis/01_intro_redis &
git-push:
	git push origin
