led_index_to_com_and_color_index = [
###
# com,colorset
### Dustbuster (side): (D1-6)
    [2,0],
    [1,0],
    [0,0],
    [2,1],
    [1,1],
    [0,1],
### Dustbuster (top): (D19-24)
    [2,6],
    [1,6],
    [0,6],
    [2,7],
    [1,7],
    [0,7],
### Frontlights: (D7-18)
    [2,2],
    [1,2],
    [0,2],
    [2,3],
    [1,3],
    [0,3],
    [2,4],
    [1,4],
    [0,4],
    [2,5],
    [1,5],
    [0,5],
### Keypad lights: (D25-27) (28,29 are DNP)
    [0,8],
    [1,8],
    [2,8],
    #[3,8],
    #[3,4],
###
]

color_index_to_row = [
    ###
    # NAME    ROW
    # RED0    14
    # RED1    15
    # RED2    18
    # RED3    22
    # RED4    0
    # RED5    3
    # RED6    7
    # RED7    10
    # RED8    26
    ###
    # GRN0    13
    # GRN1    16
    # GRN2    19
    # GRN3    23
    # GRN4    1
    # GRN5    4
    # GRN6    8
    # GRN7    9
    # GRN8    25
    ###
    # BLU0    12
    # BLU1    17
    # BLU2    20
    # BLU3    24
    # BLU4    2
    # BLU5    5
    # BLU6    6
    # BLU7    11
    # BLU8    27
    ###
    [14,13,12],
    [15,16,17],
    [18,19,20],
    [22,23,24],
    [0,1,2],
    [3,4,5],
    [7,8,6],
    [10,9,11],
    [26,25,27]
]

com_row_to_led_index = {
    0 : dict(),
    1 : dict(),
    2 : dict(),
}

for led_index in range(len(led_index_to_com_and_color_index)):
    # [0..28]
    com = led_index_to_com_and_color_index[led_index][0]
    color_index = led_index_to_com_and_color_index[led_index][1]
    for color_id in range(3):
        row = color_index_to_row[color_index][color_id]
        com_row_to_led_index[com][row] = (led_index, color_id)
        
all_mappings = []
for com in range(len(com_row_to_led_index)):
    com_mappings = []
    for row in range(28):
        if com in com_row_to_led_index and row in com_row_to_led_index[com]:
            led_id, color = com_row_to_led_index[com][row]
            mapping = '{%d, %d}' % (led_id, color)
        else:
            mapping = '{0xff, 0xff}' # dummy mapping
        #print('ht16d_col_mapping[%d][%d] = %s' % (com, row, mapping))
        com_mappings.append(mapping)
    all_mappings.append('{%s}' % (', '.join(com_mappings)))

print('{%s}' % (', '.join(all_mappings)))