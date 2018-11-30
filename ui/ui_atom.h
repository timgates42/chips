#pragma once
/*#
    # ui_atom.h

    Integrated debugging UI for atom.h

    Do this:
    ~~~C
    #define CHIPS_IMPL
    ~~~
    before you include this file in *one* C++ file to create the 
    implementation.

    Optionally provide the following macros with your own implementation
    
    ~~~C
    CHIPS_ASSERT(c)
    ~~~
        your own assert macro (default: assert(c))

    Include the following headers (and their depenencies) before including
    ui_atom.h both for the declaration and implementation.

    - atom.h
    - mem.h
    - ui_chip.h
    - ui_util.h
    - ui_m6502.h
    - ui_mc6847.h
    - ui_i8255.h
    - ui_m6522.h
    - ui_audio.h
    - ui_dasm.h
    - ui_memedit.h
    - ui_memmap.h
    - ui_kbd.h

    ## zlib/libpng license

    Copyright (c) 2018 Andre Weissflog
    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.
    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:
        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.
        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.
        3. This notice may not be removed or altered from any source
        distribution. 
#*/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* reboot callback */
typedef void (*ui_atom_boot_cb)(atom_t* sys);

typedef struct {
    atom_t* atom;
    ui_atom_boot_cb boot_cb;
} ui_atom_desc_t;

typedef struct {
    atom_t* atom;
    ui_atom_boot_cb boot_cb;
    ui_i8255_t ppi;
    ui_audio_t audio;
    ui_kbd_t kbd;
    ui_memmap_t memmap;
    ui_memedit_t memedit[4];
    ui_dasm_t dasm[4];
} ui_atom_t;

void ui_atom_init(ui_atom_t* ui, const ui_atom_desc_t* desc);
void ui_atom_discard(ui_atom_t* ui);
void ui_atom_draw(ui_atom_t* ui, double time_ms);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION (include in C++ source) ----------------------------------*/
#ifdef CHIPS_IMPL
#ifndef __cplusplus
#error "implementation must be compiled as C++"
#endif
#include <string.h> /* memset */
#ifndef CHIPS_ASSERT
    #include <assert.h>
    #define CHIPS_ASSERT(c) assert(c)
#endif
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

static void _ui_atom_draw_menu(ui_atom_t* ui, double time_ms) {
    CHIPS_ASSERT(ui && ui->atom && ui->boot_cb);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("System")) {
            if (ImGui::MenuItem("Reset")) {
                atom_reset(ui->atom);
            }
            if (ImGui::MenuItem("Cold Boot")) {
                ui->boot_cb(ui->atom);
            }
            if (ImGui::BeginMenu("Joystick")) {
                if (ImGui::MenuItem("None", 0, (ui->atom->joystick_type == ATOM_JOYSTICKTYPE_NONE))) {
                    ui->atom->joystick_type = ATOM_JOYSTICKTYPE_NONE;
                }
                if (ImGui::MenuItem("MMC", 0, (ui->atom->joystick_type == ATOM_JOYSTICKTYPE_MMC))) {
                    ui->atom->joystick_type = ATOM_JOYSTICKTYPE_MMC;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Hardware")) {
            ImGui::MenuItem("Memory Map", 0, &ui->memmap.open);
            ImGui::MenuItem("Keyboard Matrix", 0, &ui->kbd.open);
            ImGui::MenuItem("Audio Output", 0, &ui->audio.open);
            ImGui::MenuItem("m6502 CPU (TODO)");
            ImGui::MenuItem("MC6847 (TODO)");
            ImGui::MenuItem("i8255", 0, &ui->ppi.open);
            ImGui::MenuItem("m6522 (TODO)");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            if (ImGui::BeginMenu("Memory Editor")) {
                ImGui::MenuItem("Window #1", 0, &ui->memedit[0].open);
                ImGui::MenuItem("Window #2", 0, &ui->memedit[1].open);
                ImGui::MenuItem("Window #3", 0, &ui->memedit[2].open);
                ImGui::MenuItem("Window #4", 0, &ui->memedit[3].open);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Disassembler")) {
                ImGui::MenuItem("Window #1", 0, &ui->dasm[0].open);
                ImGui::MenuItem("Window #2", 0, &ui->dasm[1].open);
                ImGui::MenuItem("Window #3", 0, &ui->dasm[2].open);
                ImGui::MenuItem("Window #4", 0, &ui->dasm[3].open);
                ImGui::EndMenu();
            }
            ImGui::MenuItem("CPU Debugger (TODO)");
            ImGui::EndMenu();
        }
        ImGui::SameLine(ImGui::GetWindowWidth() - 120);
        ImGui::Text("emu: %.2fms", time_ms);
        ImGui::EndMainMenuBar();
    }
}

static uint8_t _ui_atom_mem_read(int layer, uint16_t addr, void* user_data) {
    CHIPS_ASSERT(user_data);
    atom_t* atom = (atom_t*) user_data;
    return mem_rd(&atom->mem, addr);
}

static void _ui_atom_mem_write(int layer, uint16_t addr, uint8_t data, void* user_data) {
    CHIPS_ASSERT(user_data);
    atom_t* atom = (atom_t*) user_data;
    mem_wr(&atom->mem, addr, data);
}

static const ui_chip_pin_t _ui_atom_ppi_pins[] = {
    { "D0",     0,      I8255_D0 },
    { "D1",     1,      I8255_D1 },
    { "D2",     2,      I8255_D2 },
    { "D3",     3,      I8255_D3 },
    { "D4",     4,      I8255_D4 },
    { "D5",     5,      I8255_D5 },
    { "D6",     6,      I8255_D6 },
    { "D7",     7,      I8255_D7 },
    { "CS",     9,      I8255_CS },
    { "RD",    10,      I8255_RD },
    { "WR",    11,      I8255_WR },
    { "A0",    12,      I8255_A0 },
    { "A1",    13,      I8255_A1 },
    { "PC0",   16,      I8255_PC0 },
    { "PC1",   17,      I8255_PC1 },
    { "PC2",   18,      I8255_PC2 },
    { "PC3",   19,      I8255_PC3 },
    { "PA0",   20,      I8255_PA0 },
    { "PA1",   21,      I8255_PA1 },
    { "PA2",   22,      I8255_PA2 },
    { "PA3",   23,      I8255_PA3 },
    { "PA4",   24,      I8255_PA4 },
    { "PA5",   25,      I8255_PA5 },
    { "PA6",   26,      I8255_PA6 },
    { "PA7",   27,      I8255_PA7 },
    { "PB0",   28,      I8255_PB0 },
    { "PB1",   29,      I8255_PB1 },
    { "PB2",   30,      I8255_PB2 },
    { "PB3",   31,      I8255_PB3 },
    { "PB4",   32,      I8255_PB4 },
    { "PB5",   33,      I8255_PB5 },
    { "PB6",   34,      I8255_PB6 },
    { "PB7",   35,      I8255_PB7 },
    { "PC4",   36,      I8255_PC4 },
    { "PC5",   37,      I8255_PC5 },
    { "PC6",   38,      I8255_PC6 },
    { "PC7",   39,      I8255_PC7 },
};

void ui_atom_init(ui_atom_t* ui, const ui_atom_desc_t* desc) {
    CHIPS_ASSERT(ui && desc);
    CHIPS_ASSERT(desc->atom);
    CHIPS_ASSERT(desc->boot_cb);
    ui->atom = desc->atom;
    ui->boot_cb = desc->boot_cb;
    int x = 20, y = 20, dx = 10, dy = 10;
    {
        ui_i8255_desc_t desc = {0};
        desc.title = "i8255";
        desc.i8255 = &ui->atom->ppi;
        desc.x = x;
        desc.y = y;
        UI_CHIP_INIT_DESC(&desc.chip_desc, "i8255", 40, _ui_atom_ppi_pins);
        ui_i8255_init(&ui->ppi, &desc);
    }
    x += dx; y += dy;
    {
        ui_audio_desc_t desc = {0};
        desc.title = "Audio Output";
        desc.sample_buffer = ui->atom->sample_buffer;
        desc.num_samples = ui->atom->num_samples;
        desc.x = x;
        desc.y = y;
        ui_audio_init(&ui->audio, &desc);
    }
    x += dx; y += dy;
    {
        ui_kbd_desc_t desc = {0};
        desc.title = "Keyboard Matrix";
        desc.kbd = &ui->atom->kbd;
        desc.layers[0] = "None";
        desc.layers[1] = "Shift";
        desc.layers[2] = "Ctrl";
        desc.x = x;
        desc.y = y;
        ui_kbd_init(&ui->kbd, &desc);
    }
    x += dx; y += dy;
    {
        ui_memedit_desc_t desc = {0};
        desc.layers[0] = "System";
        desc.read_cb = _ui_atom_mem_read;
        desc.write_cb = _ui_atom_mem_write;
        desc.user_data = ui->atom;
        desc.h = 120;
        static const char* titles[] = { "Memory Editor #1", "Memory Editor #2", "Memory Editor #3", "Memory Editor #4" };
        for (int i = 0; i < 4; i++) {
            desc.title = titles[i]; desc.x = x; desc.y = y;
            ui_memedit_init(&ui->memedit[i], &desc);
            x += dx; y += dy;
        }
    }
    x += dx; y += dy;
    {
        ui_memmap_desc_t desc = {0};
        desc.title = "Memory Map";
        desc.x = x;
        desc.y = y;
        desc.w = 400;
        desc.h = 64;
        ui_memmap_init(&ui->memmap, &desc);
        /* the memory map is static */
        ui_memmap_layer(&ui->memmap, "System");
        ui_memmap_region(&ui->memmap, "RAM", 0x0000, 0x3000, true);
        ui_memmap_region(&ui->memmap, "EXT RAM", 0x3000, 0x5000, true);
        ui_memmap_region(&ui->memmap, "VIDEO RAM", 0x8000, 0x2000, true);
        ui_memmap_region(&ui->memmap, "IO AREA", 0xB000, 0x1000, true);
        ui_memmap_region(&ui->memmap, "BASIC ROM 0", 0xC000, 0x1000, true);
        ui_memmap_region(&ui->memmap, "FP ROM", 0xD000, 0x1000, true);
        ui_memmap_region(&ui->memmap, "DOS ROM", 0xE000, 0x1000, true);
        ui_memmap_region(&ui->memmap, "BASIC ROM 1", 0xF000, 0x1000, true);
    }
    x += dx; y += dy;
    {
        ui_dasm_desc_t desc = {0};
        desc.layers[0] = "System";
        desc.cpu_type = UI_DASM_CPUTYPE_M6502;
        desc.start_addr = mem_rd16(&ui->atom->mem, 0xFFFC);
        desc.read_cb = _ui_atom_mem_read;
        desc.user_data = ui->atom;
        desc.w = 400;
        desc.h = 256;
        static const char* titles[4] = { "Disassembler #1", "Disassembler #2", "Disassembler #2", "Dissassembler #3" };
        for (int i = 0; i < 4; i++) {
            desc.title = titles[i]; desc.x = x; desc.y = y;
            ui_dasm_init(&ui->dasm[i], &desc);
            x += dx; y += dy;
        }
    }
}

void ui_atom_discard(ui_atom_t* ui) {
    CHIPS_ASSERT(ui && ui->atom);
    ui->atom = 0;
    ui_i8255_discard(&ui->ppi);
    ui_kbd_discard(&ui->kbd);
    ui_audio_discard(&ui->audio);
    ui_memmap_discard(&ui->memmap);
    for (int i = 0; i < 4; i++) {
        ui_memedit_discard(&ui->memedit[i]);
        ui_dasm_discard(&ui->dasm[i]);
    }
}

void ui_atom_draw(ui_atom_t* ui, double time_ms) {
    CHIPS_ASSERT(ui && ui->atom);
    _ui_atom_draw_menu(ui, time_ms);
    ui_audio_draw(&ui->audio, ui->atom->sample_pos);
    ui_kbd_draw(&ui->kbd);
    ui_i8255_draw(&ui->ppi);
    ui_memmap_draw(&ui->memmap);
    for (int i = 0; i < 4; i++) {
        ui_memedit_draw(&ui->memedit[i]);
        ui_dasm_draw(&ui->dasm[i]);
    }
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif /* CHIPS_IMPL */