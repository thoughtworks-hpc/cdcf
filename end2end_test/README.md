# End-to-end test for CDCF

End-to-end test for CDCF is build on test framework [behave](https://github.com/behave/behave) and docker

## How to run test

Requires:

- docker engine
- Python 3
- behave (1.2.6)                   - behave is behaviour-driven development, Python style
- docker (4.2.0)                   - A Python library for the Docker Engine API.

Checkout to project root and run docker build:

```shell
$ docker build -t cdcf
```

Enter `end2end_test` and run `behave` to start:

```shell
$ cd end2end_test
$ behave
```
