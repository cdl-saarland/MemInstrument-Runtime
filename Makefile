.PHONY: all clean splay sleep rt_stat softbound

all: splay sleep rt_stat softbound

clean: splay_clean sleep_clean rt_stat_clean softbound_clean

splay:
	$(MAKE) -C splay

sleep:
	$(MAKE) -C sleep

rt_stat:
	$(MAKE) -C rt_stat

softbound:
	$(MAKE) -C softbound

splay_clean:
	$(MAKE) clean -C splay

sleep_clean:
	$(MAKE) clean -C sleep

rt_stat_clean:
	$(MAKE) clean -C rt_stat

softbound_clean:
	$(MAKE) clean -C softbound
