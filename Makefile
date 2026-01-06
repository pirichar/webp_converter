# WebP Converter - macOS Application
# Build system for creating distributable .app bundle

CC = clang
CFLAGS = -std=c11 -Wall -Wextra -Wno-unused-parameter -O2 $(shell pkg-config --cflags raylib) -I/opt/homebrew/opt/webp/include
LDFLAGS = $(shell pkg-config --libs raylib) -L/opt/homebrew/opt/webp/lib -lwebp -framework Cocoa -framework IOKit -framework CoreVideo

# Directories
SRC_DIR = src
LIB_DIR = lib
BUILD_DIR = build
APP_NAME = WebPConverter
APP_BUNDLE = $(APP_NAME).app

# Source files
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/converter.c \
          $(SRC_DIR)/presets.c \
          $(SRC_DIR)/strings.c \
          $(SRC_DIR)/ui.c \
          $(LIB_DIR)/tinyfiledialogs.c

OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SOURCES)))
EXECUTABLE = $(BUILD_DIR)/webp_converter

# Library paths for bundling
RAYLIB_DYLIB = $(shell pkg-config --variable=libdir raylib)/libraylib.dylib
WEBP_DYLIB = /opt/homebrew/opt/webp/lib/libwebp.dylib

.PHONY: all clean fclean re app dmg run install-deps

all: $(EXECUTABLE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(LIB_DIR) -c $< -o $@

$(BUILD_DIR)/tinyfiledialogs.o: $(LIB_DIR)/tinyfiledialogs.c | $(BUILD_DIR)
	$(CC) -std=c11 -Wall -O2 -c $< -o $@

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# Create macOS .app bundle
app: $(EXECUTABLE)
	@echo "Creating $(APP_BUNDLE)..."
	@rm -rf $(APP_BUNDLE)
	@mkdir -p $(APP_BUNDLE)/Contents/MacOS
	@mkdir -p $(APP_BUNDLE)/Contents/Frameworks
	@mkdir -p $(APP_BUNDLE)/Contents/Resources

	# Copy executable
	@cp $(EXECUTABLE) $(APP_BUNDLE)/Contents/MacOS/$(APP_NAME)

	# Copy dylibs
	@cp $(RAYLIB_DYLIB) $(APP_BUNDLE)/Contents/Frameworks/
	@cp $(WEBP_DYLIB) $(APP_BUNDLE)/Contents/Frameworks/

	# Also copy sharpyuv dependency of libwebp
	@cp /opt/homebrew/opt/webp/lib/libsharpyuv.dylib $(APP_BUNDLE)/Contents/Frameworks/ 2>/dev/null || true

	# Make libraries writable for install_name_tool
	@chmod +w $(APP_BUNDLE)/Contents/Frameworks/*.dylib
	@chmod +w $(APP_BUNDLE)/Contents/MacOS/$(APP_NAME)

	# Fix library paths in executable (handle versioned library names)
	@install_name_tool -change $(RAYLIB_DYLIB) @executable_path/../Frameworks/libraylib.dylib $(APP_BUNDLE)/Contents/MacOS/$(APP_NAME) 2>/dev/null || true
	@install_name_tool -change /opt/homebrew/opt/raylib/lib/libraylib.550.dylib @executable_path/../Frameworks/libraylib.dylib $(APP_BUNDLE)/Contents/MacOS/$(APP_NAME) 2>/dev/null || true
	@install_name_tool -change $(WEBP_DYLIB) @executable_path/../Frameworks/libwebp.dylib $(APP_BUNDLE)/Contents/MacOS/$(APP_NAME) 2>/dev/null || true
	@install_name_tool -change /opt/homebrew/opt/webp/lib/libwebp.7.dylib @executable_path/../Frameworks/libwebp.dylib $(APP_BUNDLE)/Contents/MacOS/$(APP_NAME) 2>/dev/null || true

	# Fix library paths in libwebp (references to libsharpyuv)
	@install_name_tool -change /opt/homebrew/opt/webp/lib/libsharpyuv.0.dylib @executable_path/../Frameworks/libsharpyuv.dylib $(APP_BUNDLE)/Contents/Frameworks/libwebp.dylib 2>/dev/null || true
	@install_name_tool -change @rpath/libsharpyuv.0.dylib @executable_path/../Frameworks/libsharpyuv.dylib $(APP_BUNDLE)/Contents/Frameworks/libwebp.dylib 2>/dev/null || true

	# Fix library IDs
	@install_name_tool -id @executable_path/../Frameworks/libraylib.dylib $(APP_BUNDLE)/Contents/Frameworks/libraylib.dylib
	@install_name_tool -id @executable_path/../Frameworks/libwebp.dylib $(APP_BUNDLE)/Contents/Frameworks/libwebp.dylib
	@install_name_tool -id @executable_path/../Frameworks/libsharpyuv.dylib $(APP_BUNDLE)/Contents/Frameworks/libsharpyuv.dylib 2>/dev/null || true

	# Ad-hoc sign the app
	@codesign --force --deep --sign - $(APP_BUNDLE)

	# Create Info.plist
	@echo '<?xml version="1.0" encoding="UTF-8"?>' > $(APP_BUNDLE)/Contents/Info.plist
	@echo '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '<plist version="1.0">' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '<dict>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <key>CFBundleExecutable</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <string>$(APP_NAME)</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <key>CFBundleIdentifier</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <string>com.webpconverter.app</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <key>CFBundleName</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <string>$(APP_NAME)</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <key>CFBundlePackageType</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <string>APPL</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <key>CFBundleShortVersionString</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <string>1.0.0</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <key>CFBundleVersion</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <string>1</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <key>LSMinimumSystemVersion</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <string>11.0</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <key>NSHighResolutionCapable</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <true/>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <key>NSHumanReadableCopyright</key>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '    <string>Copyright 2025</string>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '</dict>' >> $(APP_BUNDLE)/Contents/Info.plist
	@echo '</plist>' >> $(APP_BUNDLE)/Contents/Info.plist

	@echo "$(APP_BUNDLE) created successfully!"

# Run the app bundle
run: app
	open $(APP_BUNDLE)

# Create DMG for distribution
dmg: app
	@echo "Creating DMG..."
	@rm -f $(APP_NAME).dmg
	@mkdir -p dmg_temp
	@cp -R $(APP_BUNDLE) dmg_temp/
	@ln -s /Applications dmg_temp/Applications
	@hdiutil create -volname "$(APP_NAME)" -srcfolder dmg_temp -ov -format UDZO $(APP_NAME).dmg
	@rm -rf dmg_temp
	@echo "$(APP_NAME).dmg created!"

# Sign the app (requires Apple Developer ID)
sign: app
	@echo "Signing $(APP_BUNDLE)..."
	codesign --deep --force --verify --verbose --sign "Developer ID Application" $(APP_BUNDLE)
	@echo "Signed!"

# Notarize the app (requires Apple Developer account)
notarize: dmg
	@echo "Submitting for notarization..."
	xcrun notarytool submit $(APP_NAME).dmg --keychain-profile "notarization" --wait
	xcrun stapler staple $(APP_NAME).dmg
	@echo "Notarized!"

clean:
	rm -rf $(BUILD_DIR)/*.o

fclean: clean
	rm -rf $(BUILD_DIR) $(APP_BUNDLE) $(APP_NAME).dmg dmg_temp

re: fclean all

install-deps:
	brew install raylib webp
