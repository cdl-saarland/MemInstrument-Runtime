.PHONY: all clean splay lowfat sleep rt_stat softbound

all: splay lowfat sleep rt_stat softbound

clean: splay_clean lowfat_clean sleep_clean rt_stat_clean softbound_clean

splay:
	$(MAKE) -C splay

lowfat:
	$(MAKE) -C lowfat

sleep:
	$(MAKE) -C sleep

rt_stat:
	$(MAKE) -C rt_stat

softbound:
	$(MAKE) -C softbound

splay_clean:
	$(MAKE) clean -C splay

lowfat_clean:
	$(MAKE) clean -C lowfat

sleep_clean:
	$(MAKE) clean -C sleep

rt_stat_clean:
	$(MAKE) clean -C rt_stat

softbound_clean:
	$(MAKE) clean -C softbound
