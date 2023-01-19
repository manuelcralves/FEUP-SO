CC=gcc
CFLAGS =-Wall -Wextra -Werror -pedantic

all:
	@$(CC) txt2epub.c $(CFLAGS) -o txt2epub

clean:
	@rm -rf txt2epub *.epub *.zip

run:
	@./txt2epub *.txt