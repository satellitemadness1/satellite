def main():
    i = 0
    total = 0
    my_list = []
    s = ""

    while i < 1000000:
        total = total + i
        my_list.append(i)
        s = "number " + str(i)
        i = i + 1

    print(total)
    print(len(my_list))
    print(s)


if __name__ == "__main__":
    main()
