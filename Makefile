APP_NAME = accounting
PREFIX = /opt/mine

CUR_DIR = $(shell pwd)
PACKAGE_DATA_DIR = \"$(CUR_DIR)/data\"

$(info $(PACKAGE_DATA_DIR))
default: build/$(APP_NAME)

CFLAGS := -Wall -Wextra -Wshadow -Wno-type-limits -g3 -O0 -Wpointer-arith -fvisibility=hidden

CFLAGS += -DAPP_NAME=\"$(APP_NAME)\" -DPACKAGE_DATA_DIR=$(PACKAGE_DATA_DIR)

build/$(APP_NAME): src/accounting.c src/common.c src/desc_parser.c src/html_generator.c src/history_parser.c src/list.c
	mkdir -p $(@D)
	gcc -g $^ $(CFLAGS) -o $@

install: build/$(APP_NAME)
	mkdir -p $(PREFIX)/bin
	install -c build/$(APP_NAME) $(PREFIX)/bin/

clean:
	rm -rf build/
