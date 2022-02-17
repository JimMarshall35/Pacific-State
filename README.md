# Pacific-State
a single header state machine library based on the c# lib stateless

NOT FINISHED YET - nearly 

Same fluent API as stateless, will ultimately have all the same features (has most atm).

Unlike statless you MUST Use an enum to represent states and triggers. Vectors used to store these not maps for more compact memory usage and minimal dynamic memory allocation. Done with a view to using this library in an embedded setting in the future and/or producing a plain C version (without the fluent api).

State enum MUST start from 0 with 0 being "InvalidState".

See Main.cpp for example usage
