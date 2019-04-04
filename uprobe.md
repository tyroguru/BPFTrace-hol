## Dynamic User Probes

In this lab you will experiment with probing dynamic user probes. *Dynamic* probes are probes that are not *statically* defined apriori in code (an example of this is the `USDT` probes (XXX link to that section)). Dynamic probes are discovered on-demand by the bpf sub-system. For userland application and libraries a point of instrumentation is called a `uprobe` (*user probe*).

### uprobe probe naming format

At the simplest level two probes will be created for every function in an application, one for the entry to a function and one for the return. 
---
## Exercises

---

## Further Reading
