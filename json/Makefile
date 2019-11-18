# Copyright (C) 2019 Veronika Sokova <isokova@fit.vutbr.cz>
#
# This file is part of code listener.
#
# code listener is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# code listener is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with predator.  If not, see <http://www.gnu.org/licenses/>.

CMAKE           ?= cmake
CTEST           ?= ctest

.PHONY: all check clean distclean distcheck

all: ../cl_build/Makefile
	# make sure that libcl.a is up2date
	$(MAKE) -C ../cl_build
	mkdir -p ../json_build
	cd ../json_build && $(CMAKE) ../json
	$(MAKE) -C ../json_build

../cl_build/Makefile:
	test -e $@ || $(MAKE) -C ../cl

atd:
	mkdir -p ../json_build
	./make.sh

check: all atd
	cd ../json_build && $(CTEST) --output-on-failure

clean:
	if test -e ../json_build/Makefile; then \
		$(MAKE) clean -C ../json_build; \
	fi

distclean: clean
	rm -rf ../json_build

distcheck: distclean
	$(MAKE) check CMAKE='cmake -D JSON_DEBUG=ON'
	$(MAKE) check CMAKE='cmake -D JSON_DEBUG=OFF'
