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
    return row + 6 * col + 6 * 7 * player


def get_feature(features, row, col, player):
    return features[get_feature_index(row, col, player)]


def set_feature(features, row, col, player):
    index = get_feature_index(row, col, player)
    if features[index] != 0:
        raise RuntimeError(f"Feature at index {index} must be 0, but is {features[index]}.")

    features[index] = 1


def clear_feature(features, row, col, player):
    index = get_feature_index(row, col, player)
    if features[index] != 1:
        raise RuntimeError(f"Feature at index {index} must be 1, but is {features[index]}.")

    features[index] = 0


def is_cell_empty(features, row, col):
    return get_feature(features, row, col, 0) == 0 and get_feature(features, row, col, 1) == 0


def get_top_row_in_col(features, col):
    for row in range(5, -1, -1):
        if not is_cell_empty(features, row, col):
            return row

    return -1
