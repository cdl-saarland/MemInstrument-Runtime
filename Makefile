.PHONY: all clean splay lowfat sleep rt-stat softbound example

all: splay lowfat sleep rt-stat softbound example

lto: splay-lto lowfat-lto softbound-lto

exports: lowfat-exports softbound-exports

clean: splay-clean lowfat-clean sleep-clean rt-stat-clean softbound-clean example-clean

format: splay-format lowfat-format sleep-format rt-stat-format softbound-format shared-format example-format

splay:
	$(MAKE) -C splay

lowfat:
	$(MAKE) -C lowfat

sleep:
	$(MAKE) -C sleep

rt-stat:
	$(MAKE) -C rt_stat

softbound:
	$(MAKE) -C softbound

example:
	$(MAKE) -C example

splay-lto:
	$(MAKE) lto-static -C splay

lowfat-lto:
	$(MAKE) lto-static -C lowfat

softbound-lto:
	$(MAKE) lto-static -C softbound

lowfat-exports:
	$(MAKE) exports -C lowfat

softbound-exports:
	$(MAKE) exports -C softbound

splay-clean:
	$(MAKE) clean -C splay

lowfat-clean:
	$(MAKE) clean -C lowfat

sleep-clean:
	$(MAKE) clean -C sleep

rt-stat-clean:
	$(MAKE) clean -C rt_stat

softbound-clean:
	$(MAKE) clean -C softbound

example-clean:
	$(MAKE) clean -C example

splay-format:
	$(MAKE) format -C splay

lowfat-format:
	$(MAKE) format -C lowfat

sleep-format:
	$(MAKE) format -C sleep

rt-stat-format:
	$(MAKE) format -C rt_stat

softbound-format:
	$(MAKE) format -C softbound

shared-format:
	$(MAKE) format -C shared

example-format:
	$(MAKE) format -C example
