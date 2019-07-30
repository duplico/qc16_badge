$fn=180;

difference() {
    keypad_circle();
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
        cylinder(d=83.82, h=12);
    }
}

module rj_cutout(w=16) {
    translate([-w/2,0,0]) {
        cube([w,100,5]);
        cube([w,49,15]);
    }
}