# BluePill (STM32F103C8T6) Cyphal node -- build, flash, bus monitoring.
#
#   make build         compile firmware (build.sh -> build/project.{elf,hex,bin})
#   make flash         build + load via ST-Link SWD (verify + reset)
#   make wireshark     bring can0 up if needed, then Wireshark GUI live
#                      capture with the Cyphal v1.1 Lua dissector
#   make tshark        same, but headless CLI capture (Ctrl-C to stop)
#   make getinfo       yakut call uavcan.node.GetInfo.1.0 on node $(NODE_ID)
#   make can0-up       attach the WeAct adapter as can0 (sudo, only if down)
#   make can0-down     release can0 + /dev/ttyACM0 (sudo, only if up)
#   make help          list targets
#
# Extras: FILTER='<display filter>' for wireshark/tshark,
#         SN= for a different ST-Link serial number (SN= = auto-detect).

SHELL := /bin/bash

# Firmware node we talk to, and the node-ID yakut itself uses for RPC calls.
NODE_ID       ?= 50
LOCAL_NODE_ID ?= 42

# Bus interface created by ./can0.sh (slcand, 500 kbit/s).
IFACE ?= can0

# ST-Link SWD. STM32_Programmer_CLI from PATH, CubeCLT as fallback.
# SWD_FREQ: the default clock fails with this wiring ("Unable to get core
# ID"); 100 kHz connects reliably. SWD_FREQ= uses the tool's default.
SN          ?= 37FF71064E5734367F0A1B43
SWD_FREQ    ?= 100
STM32_PROG  ?= $(or $(shell command -v STM32_Programmer_CLI 2>/dev/null),/home/danila/st/stm32cubeclt_1.18.0/STM32CubeProgrammer/bin/STM32_Programmer_CLI)
STLINK      := port=SWD $(if $(strip $(SN)),sn=$(SN)) $(if $(strip $(SWD_FREQ)),freq=$(SWD_FREQ))

BIN := build/project.bin

# GetInfo request timeout, seconds.
GETINFO_TIMEOUT ?= 3

# dumpcap is installed root:wireshark mode 754, so a session started before
# "sudo usermod -aG wireshark $USER" lacks the group.  Detect that case and
# bridge it via setuid newgrp (no password: the membership is in /etc/group);
# after a re-login the direct branch is taken.
HAVE_WG := id -nG | tr ' ' '\n' | grep -qx wireshark

.PHONY: all build flash wireshark tshark getinfo can0-up can0-down help

all: build

build:
	./build.sh

# Manual flash (ST-Link V2 via SWD): program, verify, reset.
flash: build
	$(STM32_PROG) -c $(STLINK) -w $(BIN) 0x08000000 -v -rst

can0-up:
	@if ip link show $(IFACE) >/dev/null 2>&1; then \
		echo "$(IFACE) already up"; \
	else \
		sudo ./can0.sh up; \
	fi

can0-down:
	@if ip link show $(IFACE) >/dev/null 2>&1; then \
		sudo ./can0.sh down; \
	else \
		echo "$(IFACE) already down"; \
	fi

# Live capture; Wireshark and yakut can listen on can0 at the same time.
wireshark: can0-up
	@if $(HAVE_WG); then \
		wireshark -k -i $(IFACE) $(if $(FILTER),-Y '$(FILTER)'); \
	else \
		echo "wireshark -k -i $(IFACE)$(if $(FILTER), -Y '$(FILTER)')" | newgrp wireshark; \
	fi

tshark: can0-up
	@if $(HAVE_WG); then \
		tshark -i $(IFACE) $(if $(FILTER),-Y '$(FILTER)'); \
	else \
		echo "tshark -i $(IFACE)$(if $(FILTER), -Y '$(FILTER)')" | newgrp wireshark; \
	fi

# Send GetInfo.  yakut's transport (yakut-env.sh) is anonymous, but RPC needs
# a local node-ID: append $(LOCAL_NODE_ID) to the CAN() transport expression.
# NOTE: the current firmware only publishes Heartbeat and does not serve
# GetInfo, so this times out -- the request still shows up on the bus
# (Wireshark filter: cyphalcan11.service_id == 430).
getinfo:
	@. ./yakut-env.sh && \
		YAKUT_TRANSPORT="$${YAKUT_TRANSPORT%,None)},$(LOCAL_NODE_ID))" \
		yakut call $(NODE_ID) uavcan.node.GetInfo.1.0 -T $(GETINFO_TIMEOUT) +M \
		|| { echo "getinfo: no response from node $(NODE_ID) (request was sent;"; \
		     echo "  firmware has no GetInfo server yet -- see 'make wireshark')"; \
		     exit 1; }

help:
	@sed -n 's/^#   make /make /p' $(firstword $(MAKEFILE_LIST))
