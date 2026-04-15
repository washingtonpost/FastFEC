SHELL := /bin/zsh

ZIG ?= zig

VENV := python/.venv
FIXTURE := python/tests/fixtures/13360.fec
SMOKE_OUT := /private/tmp/fastfec_make_smoke

.PHONY: help all check-zig build c-test cli-smoke py-setup py-install py-test

help:
	@echo "Targets:"
	@echo "  make build      - Build CLI + shared lib"
	@echo "  make c-test     - Run C tests"
	@echo "  make cli-smoke  - Parse fixture with CLI"
	@echo "  make py-setup   - Create python/.venv"
	@echo "  make py-install - Install python dev deps"
	@echo "  make py-test    - Run Python tox tests"
	@echo "  make all        - Run full verification"

check-zig:
	@command -v $(ZIG) >/dev/null 2>&1 || (echo "Zig not found. Install with: brew install zig" && exit 1)
	@$(ZIG) version

build: check-zig
	$(ZIG) build

c-test: check-zig
	$(ZIG) build test

cli-smoke: build
	mkdir -p $(SMOKE_OUT)
	./zig-out/bin/fastfec -x -s $(FIXTURE) $(SMOKE_OUT) 13360
	@test -d $(SMOKE_OUT)/13360
	@echo "Smoke output files:"
	@find $(SMOKE_OUT)/13360 -maxdepth 1 -type f | head -n 10

py-setup:
	@test -d $(VENV) || python3 -m venv $(VENV)

py-install: py-setup
	source $(VENV)/bin/activate && cd python && pip install -r requirements-dev.txt

py-test: py-install
	source $(VENV)/bin/activate && cd python && tox -e py

all: build c-test cli-smoke py-test
