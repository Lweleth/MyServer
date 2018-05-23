param =  -g
obj = util.c s_string.c config.c  buffer.c map.c connection.c  request.c response.c parse.c server.c main.c
test = s_test.c
map = s_string.c map.c

sock_test:  $(obj)
	gcc $(param) $(obj) -o sock_test

test: $(test)
	gcc $(param) $(test) -o xx

map: $(map)
	gcc $(param) $(map) -o map_test


clean:
	rm -rf sock_test