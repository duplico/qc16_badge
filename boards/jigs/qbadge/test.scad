intersection() {
difference() {
    translate([-62.5,-62.5,0]) {
        cube([125,125,5]);
    }
    translate([0,0,1]) {
        badge_with_clearances();
    }
}
cylinder(d=120,h=10);
}

//badge_with_clearances();

module badge_with_clearances() {
    translate([0,0,0]) {
        scale([1.005,1.005,10])
            badge();
    }
    translate([0,0,-10]) scale([1,1,10]) union() {
        translate([-38.879,16.871,0]) {
            pin_5();
        }

        translate([-29.185,-7.471,0]) {
            pin_5();
        }
    }
    minkowski() {
        union() {
            translate([0,13.138,-16.7])
                    battery_footprint();
            translate([-13.3/2,55.634-29,-11])
                cube([13.3,29,14.4]);
        }
        sphere(d=3);
    }
}

module badge() {
    import("qbadge_board.stl");
}

module pin_5() {
    $fn=24;
    for ( x = [0 : 5] ) {
        translate([x*1.41421, x*1.41421]) {
            cylinder(d=1.5, h=1);
        }
    }
}

module battery_footprint() {
    translate([-57.1/2,-32/2,0])
    cube([57.1,32,16.7]);
}

/*
Note: Pin locations are:
MCU:
-29.185,-7.471
-27.77,-6.057
-26.356,-4.642
-24.942, -3.228
-23.528,-1.814

(1.41421)

SPIF:
-38.879,16.871
-37.465,18.285
-36.051,19.699
-34.637,21.113
-33.222,22.527

Note: Lanyard holes:
-36.135,28.124
35.747,28.124

Battery pins are:
24.19,19.488
24.19,6.788
(2mm from hole to edge, I think)
*/