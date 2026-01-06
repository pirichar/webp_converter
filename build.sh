#!/bin/bash

# ============================================
#  WebP Converter - Build Script
# ============================================

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

# Symbols
CHECK="${GREEN}✓${NC}"
CROSS="${RED}✗${NC}"
ARROW="${CYAN}→${NC}"
STAR="${YELLOW}★${NC}"

# Print banner
echo ""
echo -e "${CYAN}╔═══════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║${NC}  ${WHITE}██████╗   ██╗  ██████╗${NC}   ${MAGENTA}WebP Converter${NC}            ${CYAN}║${NC}"
echo -e "${CYAN}║${NC}  ${WHITE}██╔══██╗  ██║  ██╔══██╗${NC}  ${WHITE}Build Script${NC}              ${CYAN}║${NC}"
echo -e "${CYAN}║${NC}  ${WHITE}██████╔╝  ██║  ██████╔╝${NC}                             ${CYAN}║${NC}"
echo -e "${CYAN}║${NC}  ${WHITE}██╔═══╝   ██║  ██╔══██╗${NC}  macOS Image Converter      ${CYAN}║${NC}"
echo -e "${CYAN}║${NC}  ${WHITE}██║       ██║  ██║  ██║${NC}                             ${CYAN}║${NC}"
echo -e "${CYAN}║${NC}  ${WHITE}╚═╝       ╚═╝  ╚═╝  ╚═╝${NC}                             ${CYAN}║${NC}"
echo -e "${CYAN}╚═══════════════════════════════════════════════════════╝${NC}"
echo ""

# Function to print step
step() {
    echo -e "${ARROW} ${WHITE}$1${NC}"
}

# Function to print success
success() {
    echo -e "  ${CHECK} ${GREEN}$1${NC}"
}

# Function to print error
error() {
    echo -e "  ${CROSS} ${RED}$1${NC}"
}

# Function to print warning
warn() {
    echo -e "  ${STAR} ${YELLOW}$1${NC}"
}

# Check if command exists
check_command() {
    if command -v $1 &> /dev/null; then
        return 0
    else
        return 1
    fi
}

# ============================================
#  Check Prerequisites
# ============================================

echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${WHITE}  Checking Prerequisites${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Check Xcode CLI tools
step "Checking Xcode Command Line Tools..."
if xcode-select -p &> /dev/null; then
    success "Xcode CLI tools installed"
else
    error "Xcode CLI tools not found"
    echo ""
    warn "Installing Xcode CLI tools..."
    xcode-select --install
    echo ""
    echo -e "${YELLOW}Please run this script again after installation completes.${NC}"
    exit 1
fi

# Check Homebrew
step "Checking Homebrew..."
if check_command brew; then
    success "Homebrew installed"
else
    error "Homebrew not found"
    echo ""
    warn "Install Homebrew first:"
    echo -e "    ${WHITE}/bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\"${NC}"
    exit 1
fi

# Check raylib
step "Checking raylib..."
if brew list raylib &> /dev/null; then
    success "raylib installed"
else
    warn "raylib not found - installing..."
    brew install raylib
    if [ $? -eq 0 ]; then
        success "raylib installed"
    else
        error "Failed to install raylib"
        exit 1
    fi
fi

# Check libwebp
step "Checking libwebp..."
if brew list webp &> /dev/null; then
    success "libwebp installed"
else
    warn "libwebp not found - installing..."
    brew install webp
    if [ $? -eq 0 ]; then
        success "libwebp installed"
    else
        error "Failed to install libwebp"
        exit 1
    fi
fi

echo ""

# ============================================
#  Build Options
# ============================================

echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${WHITE}  What would you like to do?${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "  ${WHITE}1)${NC} Build and Run          ${CYAN}(test locally)${NC}"
echo -e "  ${WHITE}2)${NC} Build App Bundle       ${CYAN}(WebPConverter.app)${NC}"
echo -e "  ${WHITE}3)${NC} Create ZIP             ${CYAN}(for distribution)${NC}"
echo -e "  ${WHITE}4)${NC} Create DMG             ${CYAN}(disk image)${NC}"
echo -e "  ${WHITE}5)${NC} Clean Build            ${CYAN}(remove all artifacts)${NC}"
echo -e "  ${WHITE}6)${NC} Exit"
echo ""

read -p "$(echo -e ${CYAN}"Enter choice [1-6]: "${NC})" choice

echo ""

case $choice in
    1)
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${WHITE}  Building and Running...${NC}"
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo ""
        make run
        ;;
    2)
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${WHITE}  Building App Bundle...${NC}"
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo ""
        make app
        if [ $? -eq 0 ]; then
            echo ""
            success "App bundle created: ${WHITE}WebPConverter.app${NC}"
        fi
        ;;
    3)
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${WHITE}  Creating ZIP for Distribution...${NC}"
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo ""
        make app
        if [ $? -eq 0 ]; then
            rm -f WebPConverter-app.zip
            zip -r WebPConverter-app.zip WebPConverter.app
            echo ""
            success "ZIP created: ${WHITE}WebPConverter-app.zip${NC}"
            echo ""
            echo -e "  ${STAR} ${YELLOW}To distribute:${NC}"
            echo -e "     1. Send ${WHITE}WebPConverter-app.zip${NC} to users"
            echo -e "     2. Tell them: Extract → ${WHITE}Right-click${NC} app → ${WHITE}Open${NC}"
        fi
        ;;
    4)
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${WHITE}  Creating DMG...${NC}"
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo ""
        make dmg
        if [ $? -eq 0 ]; then
            echo ""
            success "DMG created: ${WHITE}WebPConverter.dmg${NC}"
        fi
        ;;
    5)
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${WHITE}  Cleaning Build Artifacts...${NC}"
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo ""
        make fclean
        rm -f WebPConverter-app.zip
        success "All build artifacts removed"
        ;;
    6)
        echo -e "${CYAN}Goodbye!${NC}"
        exit 0
        ;;
    *)
        error "Invalid choice"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${WHITE}  Done!${NC}"
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
