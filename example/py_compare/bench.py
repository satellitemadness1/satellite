import sys


class holder:
    def __init__(self, input_str):
        self.the_name = input_str

    def set(self, input_str):
        self.the_name = input_str


def main(argz):
    rounds = 300000

    made = 0

    while made < rounds:
        scratch = holder("some_data")

        made = made + 1

    shown = 0

    while shown < rounds:
        print(shown)

        shown = shown + 1

    target = holder("some_data")

    sent = 0

    while sent < rounds:
        target.set("another_str")

        sent = sent + 1

    print("done")

    return 0


sys.exit(main(sys.argv))
