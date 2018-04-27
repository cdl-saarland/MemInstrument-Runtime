.PHONY: all clean splay rt_stat

all: splay rt_stat

clean: splay_clean rt_stat_clean

splay:
	$(MAKE) -C splay

rt_stat:
	$(MAKE) -C rt_stat

splay_clean:
	$(MAKE) clean -C splay

rt_stat_clean:
	$(MAKE) clean -C rt_stat

