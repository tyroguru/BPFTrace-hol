SUBDIRS = bpfhol kprobes syscalls usdt uprobes

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@
