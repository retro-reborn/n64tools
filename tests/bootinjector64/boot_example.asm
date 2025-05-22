.n64
.create "bootcode.bin", 0x0

//-------------------------------------------------
// Custom N64 Boot Code Example
//-------------------------------------------------

// Define stack location
.definelabel STACK_LOCATION, 0x80400000

// Entry point
start:
    // Set up stack pointer
    lui     sp, STACK_LOCATION >> 16
    
    // Simple animation loop
    li      t0, 1000     // Counter for delay
    li      t1, 0        // Pattern counter
    
animation_loop:
    // Load pattern for color bars based on t1
    li      t2, 0x80000000  // Video memory base address
    li      t3, 640*240     // Screen size (pixels)
    
    // Use t1 to select a color pattern
    andi    t4, t1, 0x7
    
    // Different pattern based on t4 value
    beqz    t4, pattern0
    nop
    li      t5, 1
    beq     t4, t5, pattern1
    nop
    li      t5, 2
    beq     t4, t5, pattern2
    nop
    
    // Default pattern - checkerboard
    b       pattern3
    nop
    
pattern0:
    // Red pattern
    li      t5, 0xFF0000FF
    b       fill_screen
    nop
    
pattern1:
    // Green pattern
    li      t5, 0x00FF00FF
    b       fill_screen
    nop
    
pattern2:
    // Blue pattern
    li      t5, 0x0000FFFF
    b       fill_screen
    nop
    
pattern3:
    // Yellow pattern
    li      t5, 0xFFFF00FF
    
fill_screen:
    // Fill screen with pattern
    sw      t5, 0(t2)
    addiu   t2, t2, 4
    addiu   t3, t3, -1
    bnez    t3, fill_screen
    nop
    
    // Delay loop
    delay_loop:
        addiu   t0, t0, -1
        bnez    t0, delay_loop
        nop
    
    // Reset delay counter and increment pattern
    li      t0, 1000
    addiu   t1, t1, 1
    
    // Loop forever
    b       animation_loop
    nop
