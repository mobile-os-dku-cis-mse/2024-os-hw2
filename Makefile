# Makefile

BUILD_DIR = build
CMAKE_MIN_VERSION = 3.22
CURRENT_DIR := $(shell pwd)


# CMake 존재 여부 및 버전 확인 함수
define check_cmake
	@if ! command -v cmake >/dev/null 2>&1; then \
		echo "Error: CMake is not installed."; \
		echo "Install CMake over than ver. $(CMAKE_MIN_VERSION) "; \
		exit 1; \
	else \
		CMAKE_VERSION=$$(cmake --version | head -n1 | cut -d" " -f3); \
		REQUIRED_VERSION=$(CMAKE_MIN_VERSION); \
		if [ $$(printf '%s\n' "$$REQUIRED_VERSION" "$$CMAKE_VERSION" | sort -V | head -n1) != "$$REQUIRED_VERSION" ]; then \
			echo "Error: Installed CMake version ($$CMAKE_VERSION) is too old."; \
			echo "Install CMake over than ver. $(CMAKE_MIN_VERSION)"; \
			exit 1; \
		fi; \
	fi
endef

.PHONY: all clean

all: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)
	@echo "======================================"
	@echo "🎉 success to build"
	@echo "execute file location: ./build/prod_cond (single buffer)"
	@echo "execute file location: ./build/single_share_queue (single share queue)"
	@echo "execute file location: ./build/shared_queue (multi share queue)"
	@echo "execute file location: ./build/char_stat (char_stat : verifyer)"
	@echo "======================================"

$(BUILD_DIR)/Makefile: CMakeLists.txt
	$(call check_cmake)
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ..



clean:
	rm -rf $(BUILD_DIR)

