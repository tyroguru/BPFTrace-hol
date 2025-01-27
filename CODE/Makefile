SUBDIRS = bpfhol core kprobes syscalls usdt uprobes

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@
