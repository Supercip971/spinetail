# Protocol 


Spinetail opens a file and reads line by line.

It start by skipping every token until it finds: 

```
@st:begin
```
It is used to indicate the start of the spinetail protocol.

The protocol will read until the end of the file or until it finds:

```
@st:end
```

It is used to indicate the end of the spinetail protocol.


You can then create an event, each event starts with 
```
@st:event 
```

You can then create attribute for each line: 

```
@st:event
name "gui-demo" 
pid 10 
task gui-demo 
#group 0 
#time 100ns
```

`#` attributes are specific to spinetail. They are hardcoded and used to provide spinetail specific metadata about the event.
- `#group` - the group the event belongs to (eg: cpu)
- `#tick` - the tick the event occurred on (floating point/integer)
- `#name` - the name of the event
- [NOT IMPLEMENTED] `#ref` - the asset id of the event (tasks, ...)
- `#kind` - the kind of the event
  - lock: => span exists for one frame  
  - scheduler (ex: context switch) => span exist until next scheduler event
  - API call (ex: syscall) => span exists for one frame  

You can then have a file with: 
```c
@st:event 
#name kernel
#kind scheduler
#pid 10
#tick 0
#group 1
@st:event
#name gui-demo 
#kind scheduler
#pid 10
#tick 0
#group 0
@st:event
#name disk 
#kind scheduler
#pid 10
#tick 1
#group 0
@st:event 
#name render 
#kind scheduler
#pid 10
#tick 2
#group 0
@st:event 
#name vfs 
#kind scheduler
#pid 10
#tick 2
#group 1
```
