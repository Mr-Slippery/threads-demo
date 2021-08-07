# Worker thread controller problem

## Design

There is a controller thread and a number of "worker" threads, although there is no explicit work queue yet. This would be a next step going forward to real-world use-cases.

The workers have run-states and processing states.

The control thread can both read and change the run-states, but it can only read the processing states.

The workers can change their own run-state and processing state.

The run-states and processing states are protected from data races with a vector of mutexes and associated condition variables shared between the control thread and the workers.

## Computational payload

The payload is trivial to keep the implementation time reasonable: the two types of workers are incrementing and decrementing an initial number. But they could be fairly easily applicable to concurrent rendering of fractals, especially Buddhabrot types, which is on my roadmap.

## Error handling

Most errors in command-line arguments and runtime input are handled.

Some unhandled errors and conditions are:
* too many threads requested - not handled because the exception should be informative enough and there is no reasonable way to continue.
* signals such as SIGINT/Ctrl-C - not handled (mainly to decrease platform-specific code)

## Extra features

A `sleep` command was introduced as an extra feature for testing. This instructs the control thread to sleep for a number of seconds, givin the workers time to progress. This is especially useful if the runtime commands are not entered interactively but from a file, such as `test_package/program.thd`. A related feature is that in the case of non-interactive terminals on a Unix platform, the program will echo each command to make the output understandable.

If the `exit` command is issued and there are non-`finished` workers (`paused` or `running`), these are counted, and after workers are instructed to terminate, the count of the count of these remaining workers terminated by `exit` is displayed. This is especially useful when dealing with a large number of workers.

## Building, packaging and running

The `compute` library, with its trivial payload, is the main Conan package.

The application is implemented, for convenience, as a `test_package` of this library.

This allows building with a single Conan command:

`conan create .`

After the build is done some trivial tests for the `compute` library are run and at the end the actual application is run on the `test_package\program.thd` input file.
The output should be similar to:

```
Running demo with 3 threads...
> status
1 running 0
2 running 0
3 running 0
> sleep 2
> pause 1
> sleep 2
> status
1 paused -9
2 running 19
3 running -19
> sleep 2
> stop 3
> sleep 2
> status
1 paused -9
2 running 39
3 finished -29
> stop 1
> sleep 2
> status
1 finished -9
2 running 49
3 finished -29
> exit
Stopping 1 remaining worker(s).
```

To run the application afterwards (on a Linux), enter e.g.:

`./test_package/build/<build-hash>/bin/thread_demo --threads 3`