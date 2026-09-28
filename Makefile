# CANbus4BluePill -- one Makefile for all eight BluePill Cyphal projects.
# Each project keeps its own Makefile with the full toolset (files shared by
# all of them -- can0.sh, Drivers/, lib/, DSDLtypes/, the helpers -- live in
# this directory and are symlinked into the projects); this root file only
# dispatches, so any per-project flag (FILTER=, SN=, SWD_FREQ=,
# GETINFO_TIMEOUT=, NODE_ID=, ...) still works.
#
#   make                 build ALL projects (default target)
#   make build-<name>    build one project,    e.g. make build-flywheel
#   make flash-<name>    flash one project,    e.g. make flash-central
#   make getinfo-<name>  yakut GetInfo on that project's node-ID
#   make clean-<name>    remove one project's build/ directory
#   make clean           remove every build/ directory
#   make can0-up         attach the WeAct adapter as can0 (sudo, only if down)
#   make can0-down       release can0 + adapter (sudo, only if up)
#   make can0-status     show can0 + adapter state (no sudo)
#   make wireshark       bring can0 up if needed, then Wireshark GUI live
#                        capture with the Cyphal v1.1 Lua dissector
#   make tshark          same, but headless CLI capture (Ctrl-C to stop)
#   make list            name -> project directory -> node-ID
#   make help            list targets
#
# <name> is a project alias (see `make list`); a full directory name works
# too, e.g. make build-STM32F103_GNSS.

SHELL := /bin/bash
.DEFAULT_GOAL := all

# Project alias -> directory.  Single source of truth for dispatch/list/help.
ALIASES := template solar imu flywheel coil gnss temp central

dir_template := STM32F103_CAN
dir_solar    := STM32F103_SolarSensor
dir_imu      := STM32F103_IMU_CAN
dir_flywheel := STM32F103_Flywheel
dir_coil     := STM32F103_Coil
dir_gnss     := STM32F103_GNSS
dir_temp     := STM32F103_TemperatureSensor
dir_central  := STM32F103_CentralProcessor

# $(call proj_dir,<name>): alias first, then a literal project directory name.
proj_dir = $(or $(dir_$(1)),$(if $(wildcard $(1)/Makefile),$(1)))
# Recipe guard: abort before running anything for an unknown <name> ($* is
# the pattern stem inside a recipe).
guard    = $(if $(call proj_dir,$*),,$(error unknown project '$*' -- aliases: $(ALIASES), or a directory like STM32F103_Flywheel))

.PHONY: all build clean list help flash getinfo \
	can0-up can0-down can0-status wireshark tshark

all: build

# Building everything = building every alias (use -j to parallelize).
build: $(addprefix build-,$(ALIASES))

# Per-project dispatch.  NOTE: these are pattern rules and therefore must NOT
# be listed in .PHONY -- make skips the implicit-rule search for phony targets
# and would find no recipe at all.
build-%:
	$(guard)
	@echo "==> build $* ($(call proj_dir,$*))"
	$(MAKE) -C $(call proj_dir,$*) build

flash-%:
	$(guard)
	$(MAKE) -C $(call proj_dir,$*) flash

getinfo-%:
	$(guard)
	$(MAKE) -C $(call proj_dir,$*) getinfo

clean-%:
	$(guard)
	rm -rf $(call proj_dir,$*)/build

# flash/getinfo need a project: only one board sits on the ST-Link at a time.
flash getinfo:
	@echo "usage: make $(@)-<name>   where <name> is one of:" >&2
	@echo "  $(ALIASES)   (or a directory like STM32F103_Flywheel)" >&2
	@exit 2

# The bus tooling (can0.sh) lives in the repository root and is symlinked
# into every project; up/down are guarded here so sudo is only asked when
# the interface state actually has to change (IFACE is can0 inside can0.sh).
can0-up:
	@if ip link show can0 >/dev/null 2>&1; then \
		echo "can0 already up"; \
	else \
		sudo ./can0.sh up; \
	fi

can0-down:
	@if ip link show can0 >/dev/null 2>&1; then \
		sudo ./can0.sh down; \
	else \
		echo "can0 already down"; \
	fi

can0-status:
	@./can0.sh status

wireshark:
	$(MAKE) -C $(dir_template) wireshark

tshark:
	$(MAKE) -C $(dir_template) tshark

clean: $(addprefix clean-,$(ALIASES))

list:
	@$(foreach a,$(ALIASES),printf '%-9s %-30s node %s\n' '$(a)' '$(dir_$(a))' "$$(sed -n 's/^NODE_ID[^0-9]*\([0-9][0-9]*\).*/\1/p' $(dir_$(a))/Makefile)";)

help:
	@sed -n 's/^#   make /make /p' $(firstword $(MAKEFILE_LIST))
	@echo "  <name> = $(ALIASES)  ('make list' for directories + node-IDs)"
