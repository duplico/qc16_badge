$fn=180;


difference() {
    keypad_circle();
    scale([1,1,0.75])
    import("qbadge_board.stl");
    
    scale([1,1,10]) translate([0,-100,0]) {
        rotate([0,0,180]) {
            import("qbadge_board.stl");
        }
    }
    
    rj_cutout();
}


module keypad_circle() {
    difference() {
        cylinder(d=110, h=10);
        scale([10.1,10.1]) linear_extrude(15) import("keypad_rB0_outline_v1.dxf");
    }
}

module rj_cutout(w=16) {
    translate([-w/2,0,0]) {
        cube([w,100,5]);
        cube([w,49,15]);
    }
}