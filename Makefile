all:
	gcc -g -O0  virt2phy.c -o virt2phy
	gcc -g -O0  virt2phy2.c -o virt2phy2
clean:
	rm *.out