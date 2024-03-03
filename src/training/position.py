from settings import BOARD_WIDTH, BOARD_HEIGHT, BOARD_AREA


def print_feature(features):
    for row in range(BOARD_HEIGHT - 1, -1, -1):
        line = ""
        for col in range(BOARD_WIDTH):
            if features[row + col * BOARD_HEIGHT] == 1:
                line += " \x1B[31mO\033[0m"
            elif features[row + col * BOARD_HEIGHT + BOARD_AREA] == 1:
                line += " \x1B[33mX\033[0m"
            else:
                line += " ."

        print(line)

    footer = ""
    for i in range(BOARD_WIDTH):
        footer += f" {i}"
    print(footer)


def get_feature_index(row, col, player):
    return row + BOARD_HEIGHT * col + BOARD_AREA * player


def get_feature(features, row, col, player):
    assert 0 <= row < BOARD_HEIGHT
    assert 0 <= col < BOARD_WIDTH
    assert player == 0 or player == 1

    return features[get_feature_index(row, col, player)]


def set_feature(features, row, col, player):
    assert 0 <= row < BOARD_HEIGHT
    assert 0 <= col < BOARD_WIDTH
    assert player == 0 or player == 1

    index = get_feature_index(row, col, player)
    if features[index] != 0:
        raise RuntimeError(f"Feature at index {index} must be 0, but is {features[index]}.")

    features[index] = 1


def is_cell_empty(features, row, col):
    return get_feature(features, row, col, 0) == 0 and get_feature(features, row, col, 1) == 0


def move(features, col):
    assert 0 <= col < BOARD_WIDTH

    for row in range(BOARD_HEIGHT):
        if is_cell_empty(features, row, col):
            set_feature(features, row, col, 0)
            return

    assert False


def get_valid_moves(features):
    valid_moves = []
    for col in range(BOARD_WIDTH):
        if is_cell_empty(features, BOARD_HEIGHT - 1, col):
            valid_moves.append(col)

    return valid_moves
