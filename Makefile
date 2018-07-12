.PHONY: all clean splay rt_stat

all: splay sleep rt_stat

clean: splay_clean sleep_clean rt_stat_clean

splay:
	$(MAKE) -C splay

sleep:
	$(MAKE) -C sleep

rt_stat:
	$(MAKE) -C rt_stat

splay_clean:
	$(MAKE) clean -C splay

sleep_clean:
	$(MAKE) clean -C sleep


rt_stat_clean:
	$(MAKE) clean -C rt_stat

