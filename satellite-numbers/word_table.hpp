#pragma once
// satellite-numbers/word_table.hpp -- THE WORDS BUILT INTO satl, BY NUMBER (the author, 2026-10-07:
// "We were supposed to have built the satellite-numbers directly into the interpreter, so they are
// not external ... make a table that calls a certain function for each number, and for each number,
// just copy over that source code into the function that corresponds to each number").
//
// EVERY WORD THAT WAS A .so UNDER satellite-numbers/<word>/ IS A FILE UNDER satellite-numbers/words/,
// named by its numbers (1.6.1.5.cpp), compiled into satl like any other source, and ends in ONE
// function named by the same numbers: describe_1_6_1_5. The table below is what call_number.
// satellite.cpp walks at start-up, calling each function to fill a LibraryRow exactly as
// dlsym("satellite_number_describe") was called on each library until this day -- so nothing is
// opened, nothing is found beside the binary, and satl alone is the whole language. The 33
// arguments words went the same way on 2026-10-03 (satellite/arguments/argument_words.hpp).
//
// A NEW WORD IS A NEW FILE AND A NEW ROW HERE -- and one more name in WORD_SOURCES
// (make_support/040-sources.mk), which names every file one by one. The row's name and numbers are
// checked against words/words.tsv as satl starts, so a row cannot quietly disagree with the table.

#include "number_row.hpp"

#include <cstddef>

namespace satellite004 {

namespace built_in {

signed long long int describe_1_4_1_1(LibraryRow *row);   // satellite.container.map.set(k, v)
signed long long int describe_1_4_1_2(LibraryRow *row);   // satellite.container.map.get(k)
signed long long int describe_1_4_1_3(LibraryRow *row);   // satellite.container.map.has(k)
signed long long int describe_1_4_1_4(LibraryRow *row);   // satellite.container.map.size
signed long long int describe_1_4_1_5(LibraryRow *row);   // satellite.container.map.empty
signed long long int describe_1_4_1_6(LibraryRow *row);   // satellite.container.map.clear
signed long long int describe_1_4_1_7(LibraryRow *row);   // satellite.container.map.remove(k)
signed long long int describe_1_4_1_8(LibraryRow *row);   // satellite.container.map.keys
signed long long int describe_1_4_1_9(LibraryRow *row);   // satellite.container.map.values
signed long long int describe_1_4_1_11(LibraryRow *row);   // satellite.container.map.first
signed long long int describe_1_4_1_12(LibraryRow *row);   // satellite.container.map.last
signed long long int describe_1_4_1_13(LibraryRow *row);   // satellite.container.map.sort(by)
signed long long int describe_1_4_1_14(LibraryRow *row);   // satellite.container.map.reverse
signed long long int describe_1_4_1_15(LibraryRow *row);   // satellite.container.map.remove_first()
signed long long int describe_1_4_1_16(LibraryRow *row);   // satellite.container.map.remove_last()
signed long long int describe_1_4_2_1(LibraryRow *row);   // satellite.container.list.append
signed long long int describe_1_4_2_2(LibraryRow *row);   // satellite.container.list.size
signed long long int describe_1_4_2_3(LibraryRow *row);   // satellite.container.list.sort()
signed long long int describe_1_4_2_8(LibraryRow *row);   // satellite.container.list.contains(x)
signed long long int describe_1_4_2_9(LibraryRow *row);   // satellite.container.list.index_of(x)
signed long long int describe_1_4_2_10(LibraryRow *row);   // satellite.container.list.empty
signed long long int describe_1_4_2_11(LibraryRow *row);   // satellite.container.list.clear
signed long long int describe_1_4_2_12(LibraryRow *row);   // satellite.container.list.first
signed long long int describe_1_4_2_13(LibraryRow *row);   // satellite.container.list.last
signed long long int describe_1_4_2_14(LibraryRow *row);   // satellite.container.list.truncate(n)
signed long long int describe_1_4_2_15(LibraryRow *row);   // satellite.container.list.reserve(n)
signed long long int describe_1_4_2_16(LibraryRow *row);   // satellite.container.list.remove_first()
signed long long int describe_1_4_2_17(LibraryRow *row);   // satellite.container.list.remove_last()
signed long long int describe_1_4_2_18(LibraryRow *row);   // satellite.container.list.remove_at(n)
signed long long int describe_1_4_2_19(LibraryRow *row);   // satellite.container.list.remove(x)
signed long long int describe_1_4_2_20(LibraryRow *row);   // satellite.container.list.insert(n, x)
signed long long int describe_1_4_2_21(LibraryRow *row);   // satellite.container.list.join(separator)
signed long long int describe_1_4_2_22(LibraryRow *row);   // satellite.container.list.reverse
signed long long int describe_1_4_2_23(LibraryRow *row);   // satellite.container.list.sum
signed long long int describe_1_4_2_24(LibraryRow *row);   // satellite.container.list.max
signed long long int describe_1_4_2_25(LibraryRow *row);   // satellite.container.list.min
signed long long int describe_1_4_2_26(LibraryRow *row);   // satellite.container.list.search(pattern)
signed long long int describe_1_4_2_27(LibraryRow *row);   // satellite.container.list.by_name
signed long long int describe_1_4_2_28(LibraryRow *row);   // satellite.container.list.by_value
signed long long int describe_1_5_1(LibraryRow *row);   // satellite.console.display
signed long long int describe_1_5_1_1(LibraryRow *row);   // satellite.console.display.center
signed long long int describe_1_5_1_2(LibraryRow *row);   // satellite.console.display.centre
signed long long int describe_1_6_1_0(LibraryRow *row);   // satellite.variable.string()
signed long long int describe_1_6_1_1(LibraryRow *row);   // satellite.variable.string.size
signed long long int describe_1_6_1_2(LibraryRow *row);   // satellite.variable.string.empty
signed long long int describe_1_6_1_3(LibraryRow *row);   // satellite.variable.string.find(x)
signed long long int describe_1_6_1_4(LibraryRow *row);   // satellite.variable.string.contains(x)
signed long long int describe_1_6_1_5(LibraryRow *row);   // satellite.variable.string.substring(start, end)
signed long long int describe_1_6_1_6(LibraryRow *row);   // satellite.variable.string.starts_with(x)
signed long long int describe_1_6_1_7(LibraryRow *row);   // satellite.variable.string.ends_with(x)
signed long long int describe_1_6_1_8(LibraryRow *row);   // satellite.variable.string.lower
signed long long int describe_1_6_1_9(LibraryRow *row);   // satellite.variable.string.upper
signed long long int describe_1_6_1_10(LibraryRow *row);   // satellite.variable.string.split(separator)
signed long long int describe_1_6_1_11(LibraryRow *row);   // satellite.variable.string.trim
signed long long int describe_1_6_1_12(LibraryRow *row);   // satellite.variable.string.replace(a, b)
signed long long int describe_1_6_1_13(LibraryRow *row);   // satellite.variable.string.to_number
signed long long int describe_1_6_1_14(LibraryRow *row);   // satellite.variable.string.append(x)
signed long long int describe_1_6_1_15(LibraryRow *row);   // satellite.variable.string.clear
signed long long int describe_1_6_1_16(LibraryRow *row);   // satellite.variable.string.at(n)
signed long long int describe_1_6_1_17(LibraryRow *row);   // satellite.variable.string.resolved
signed long long int describe_1_6_1_18(LibraryRow *row);   // satellite.variable.string(x)
signed long long int describe_1_6_1_19(LibraryRow *row);   // satellite.variable.string.string
signed long long int describe_1_6_1_20(LibraryRow *row);   // satellite.variable.string.number
signed long long int describe_1_6_1_21(LibraryRow *row);   // satellite.variable.string.binary
signed long long int describe_1_6_1_22(LibraryRow *row);   // satellite.variable.string.hex
signed long long int describe_1_6_1_23(LibraryRow *row);   // satellite.variable.string.reverse
signed long long int describe_1_6_1_24(LibraryRow *row);   // satellite.variable.string.foreground(color)
signed long long int describe_1_6_1_25(LibraryRow *row);   // satellite.variable.string.background(color)
signed long long int describe_1_6_1_26(LibraryRow *row);   // satellite.variable.string.add(x)
signed long long int describe_1_6_2_2(LibraryRow *row);   // satellite.variable.file.open
signed long long int describe_1_6_2_5(LibraryRow *row);   // satellite.variable.file.read_all
signed long long int describe_1_6_2_6(LibraryRow *row);   // satellite.variable.file.close
signed long long int describe_1_6_2_7(LibraryRow *row);   // satellite.variable.file.exists
signed long long int describe_1_6_2_8(LibraryRow *row);   // satellite.variable.file.ok
signed long long int describe_1_6_2_9(LibraryRow *row);   // satellite.variable.file.path
signed long long int describe_1_6_2_10(LibraryRow *row);   // satellite.variable.file.error
signed long long int describe_1_6_2_12(LibraryRow *row);   // satellite.variable.file.append(x)
signed long long int describe_1_6_2_13(LibraryRow *row);   // satellite.variable.file.insert(n, x)
signed long long int describe_1_6_2_14(LibraryRow *row);   // satellite.variable.file.replace(a, b)
signed long long int describe_1_6_2_15(LibraryRow *row);   // satellite.variable.file.index_of(x)
signed long long int describe_1_6_2_16(LibraryRow *row);   // satellite.variable.file.search(x)
signed long long int describe_1_6_2_17(LibraryRow *row);   // satellite.variable.file.contains(x)
signed long long int describe_1_6_2_18(LibraryRow *row);   // satellite.variable.file.remove_at(n)
signed long long int describe_1_6_2_19(LibraryRow *row);   // satellite.variable.file.remove(x)
signed long long int describe_1_6_2_20(LibraryRow *row);   // satellite.variable.file.remove_first
signed long long int describe_1_6_2_21(LibraryRow *row);   // satellite.variable.file.remove_last
signed long long int describe_1_6_2_22(LibraryRow *row);   // satellite.variable.file.truncate(n)
signed long long int describe_1_6_2_23(LibraryRow *row);   // satellite.variable.file.clear
signed long long int describe_1_6_2_24(LibraryRow *row);   // satellite.variable.file.size
signed long long int describe_1_6_2_25(LibraryRow *row);   // satellite.variable.file.empty
signed long long int describe_1_6_2_26(LibraryRow *row);   // satellite.variable.file.first
signed long long int describe_1_6_2_27(LibraryRow *row);   // satellite.variable.file.last
signed long long int describe_1_6_2_28(LibraryRow *row);   // satellite.variable.file.save
signed long long int describe_1_6_2_29(LibraryRow *row);   // satellite.variable.file.lock
signed long long int describe_1_6_2_30(LibraryRow *row);   // satellite.variable.file.unlock
signed long long int describe_1_6_13_1(LibraryRow *row);   // satellite.variable.thread.start()
signed long long int describe_1_6_13_2(LibraryRow *row);   // satellite.variable.thread.join()
signed long long int describe_1_6_13_3(LibraryRow *row);   // satellite.variable.thread.stop()
signed long long int describe_1_6_13_4(LibraryRow *row);   // satellite.variable.thread.wait()
signed long long int describe_1_6_23_1(LibraryRow *row);   // satellite.variable.program.start()
signed long long int describe_1_6_23_2(LibraryRow *row);   // satellite.variable.program.ok()
signed long long int describe_1_6_23_3(LibraryRow *row);   // satellite.variable.program.error()
signed long long int describe_1_6_23_4(LibraryRow *row);   // satellite.variable.program.join()
signed long long int describe_1_6_23_5(LibraryRow *row);   // satellite.variable.program.code()
signed long long int describe_1_6_23_6(LibraryRow *row);   // satellite.variable.program.return()
signed long long int describe_1_6_23_7(LibraryRow *row);   // satellite.variable.program.start(how)
signed long long int describe_1_6_23_8(LibraryRow *row);   // satellite.variable.program.end()
signed long long int describe_1_6_23_9(LibraryRow *row);   // satellite.variable.program.exit()
signed long long int describe_1_6_23_10(LibraryRow *row);   // satellite.variable.program.quit()
signed long long int describe_1_6_23_11(LibraryRow *row);   // satellite.variable.program.shutdown()
signed long long int describe_1_6_23_12(LibraryRow *row);   // satellite.variable.program.pass(text)
signed long long int describe_1_6_24_1(LibraryRow *row);   // satellite.variable.bash.start()
signed long long int describe_1_6_24_2(LibraryRow *row);   // satellite.variable.bash.ok()
signed long long int describe_1_6_24_3(LibraryRow *row);   // satellite.variable.bash.error()
signed long long int describe_1_6_24_4(LibraryRow *row);   // satellite.variable.bash.join()
signed long long int describe_1_6_24_5(LibraryRow *row);   // satellite.variable.bash.code()
signed long long int describe_1_6_24_6(LibraryRow *row);   // satellite.variable.bash.return()
signed long long int describe_1_6_24_7(LibraryRow *row);   // satellite.variable.bash.start(how)
signed long long int describe_1_6_24_8(LibraryRow *row);   // satellite.variable.bash.end()
signed long long int describe_1_6_24_9(LibraryRow *row);   // satellite.variable.bash.exit()
signed long long int describe_1_6_24_10(LibraryRow *row);   // satellite.variable.bash.quit()
signed long long int describe_1_6_24_11(LibraryRow *row);   // satellite.variable.bash.shutdown()
signed long long int describe_1_6_24_12(LibraryRow *row);   // satellite.variable.bash.pass(text)
signed long long int describe_1_10_1(LibraryRow *row);   // satellite.spacesuit.pointer()
signed long long int describe_1_10_2(LibraryRow *row);   // satellite.spacesuit.reference()
signed long long int describe_1_10_3(LibraryRow *row);   // satellite.spacesuit.ok()
signed long long int describe_1_10_4(LibraryRow *row);   // satellite.spacesuit.lock()
signed long long int describe_1_10_5(LibraryRow *row);   // satellite.spacesuit.unlock()
signed long long int describe_1_6_18_1(LibraryRow *row);   // satellite.variable.window.append(piece, across, down)
signed long long int describe_1_6_18_2(LibraryRow *row);   // satellite.variable.window.open()
signed long long int describe_1_6_18_3(LibraryRow *row);   // satellite.variable.window.close()
signed long long int describe_1_6_18_4(LibraryRow *row);   // satellite.variable.window.focus()
signed long long int describe_1_6_18_5(LibraryRow *row);   // satellite.variable.window.title(text)
signed long long int describe_1_6_18_6(LibraryRow *row);   // satellite.variable.window.pressed(capsule)
signed long long int describe_1_6_18_7(LibraryRow *row);   // satellite.variable.window.press()
signed long long int describe_1_6_18_8(LibraryRow *row);   // satellite.variable.window.text(words)
signed long long int describe_1_6_18_9(LibraryRow *row);   // satellite.variable.window.on(on)
signed long long int describe_1_6_18_10(LibraryRow *row);   // satellite.variable.window.value(x)
signed long long int describe_1_6_18_11(LibraryRow *row);   // satellite.variable.window.chosen(item)
signed long long int describe_1_6_18_12(LibraryRow *row);   // satellite.variable.window.path(file)
signed long long int describe_1_6_18_13(LibraryRow *row);   // satellite.variable.window.changed(capsule)
signed long long int describe_1_6_18_14(LibraryRow *row);   // satellite.variable.window.closed(capsule)
signed long long int describe_1_6_18_15(LibraryRow *row);   // satellite.variable.window.resize(wide, tall)
signed long long int describe_1_6_18_16(LibraryRow *row);   // satellite.variable.window.width
signed long long int describe_1_6_18_17(LibraryRow *row);   // satellite.variable.window.height
signed long long int describe_1_6_18_18(LibraryRow *row);   // satellite.variable.window.fullscreen(on)
signed long long int describe_1_6_18_19(LibraryRow *row);   // satellite.variable.window.colour(colour)
signed long long int describe_1_6_18_20(LibraryRow *row);   // satellite.variable.window.background(colour)
signed long long int describe_1_6_18_21(LibraryRow *row);   // satellite.variable.window.font(face, size)
signed long long int describe_1_6_18_22(LibraryRow *row);   // satellite.variable.window.every(capsule, milliseconds)
signed long long int describe_1_6_18_23(LibraryRow *row);   // satellite.variable.window.key(capsule)
signed long long int describe_1_6_18_24(LibraryRow *row);   // satellite.variable.window.clicked(capsule)
signed long long int describe_1_6_18_25(LibraryRow *row);   // satellite.variable.window.message(saying)
signed long long int describe_1_6_18_26(LibraryRow *row);   // satellite.variable.window.ask(capsule, question)
signed long long int describe_1_6_18_27(LibraryRow *row);   // satellite.variable.window.choose_a_file(capsule)
signed long long int describe_1_6_18_28(LibraryRow *row);   // satellite.variable.window.answer
signed long long int describe_1_6_18_29(LibraryRow *row);   // satellite.variable.window.menu(a_menu)
signed long long int describe_1_6_18_30(LibraryRow *row);   // satellite.variable.window.item(capsule, words)
signed long long int describe_1_6_18_31(LibraryRow *row);   // satellite.variable.window.separator()
signed long long int describe_1_6_18_32(LibraryRow *row);   // satellite.variable.window.line(from_across, from_down, to_across, to_down)
signed long long int describe_1_6_18_33(LibraryRow *row);   // satellite.variable.window.box(across, down, wide, tall)
signed long long int describe_1_6_18_34(LibraryRow *row);   // satellite.variable.window.circle(across, down, radius)
signed long long int describe_1_6_18_35(LibraryRow *row);   // satellite.variable.window.write(across, down, words)
signed long long int describe_1_6_18_36(LibraryRow *row);   // satellite.variable.window.arc(across, down, radius, from_degrees, to_degrees)
signed long long int describe_1_6_18_37(LibraryRow *row);   // satellite.variable.window.thickness(pixels)
signed long long int describe_1_6_18_38(LibraryRow *row);   // satellite.variable.window.outline(on)
signed long long int describe_1_6_18_39(LibraryRow *row);   // satellite.variable.window.across
signed long long int describe_1_6_18_40(LibraryRow *row);   // satellite.variable.window.down
signed long long int describe_1_6_18_41(LibraryRow *row);   // satellite.variable.window.clear()
signed long long int describe_1_6_18_42(LibraryRow *row);   // satellite.variable.window.save(path)
signed long long int describe_1_6_18_43(LibraryRow *row);   // satellite.variable.window.display(words)
signed long long int describe_1_6_18_44(LibraryRow *row);   // satellite.variable.window.typed(capsule)
signed long long int describe_1_6_18_45(LibraryRow *row);   // satellite.variable.window.home()
signed long long int describe_1_6_18_46(LibraryRow *row);   // satellite.variable.window.columns
signed long long int describe_1_6_18_47(LibraryRow *row);   // satellite.variable.window.rows
signed long long int describe_1_6_18_48_1(LibraryRow *row);   // satellite.variable.window.scroll.vertical(on)
signed long long int describe_1_6_18_48_2(LibraryRow *row);   // satellite.variable.window.scroll.horizontal(on)
signed long long int describe_1_6_18_50(LibraryRow *row);   // satellite.variable.window.directory(path)
signed long long int describe_1_6_18_51(LibraryRow *row);   // satellite.variable.window.listen(capsule)
signed long long int describe_1_6_18_53(LibraryRow *row);   // satellite.variable.window.ok
signed long long int describe_1_6_18_54_1(LibraryRow *row);   // satellite.variable.window.window.section(name, width, height)
signed long long int describe_1_6_18_54_2_1(LibraryRow *row);   // satellite.variable.window.window.display.file(name)
signed long long int describe_1_6_18_54_2_2_1(LibraryRow *row);   // satellite.variable.window.window.display.text.input(name)
signed long long int describe_1_6_3_1(LibraryRow *row);   // satellite.variable.time.string
signed long long int describe_1_6_5_1(LibraryRow *row);   // satellite.variable.binary.to_number
signed long long int describe_1_6_5_2(LibraryRow *row);   // satellite.variable.binary.width
signed long long int describe_1_6_5_3(LibraryRow *row);   // satellite.variable.binary.to_string
signed long long int describe_1_6_5_4(LibraryRow *row);   // satellite.variable.binary.as_number
signed long long int describe_1_6_5_5(LibraryRow *row);   // satellite.variable.binary.digits
signed long long int describe_1_6_5_6(LibraryRow *row);   // satellite.variable.binary.to_hex
signed long long int describe_1_6_5_8(LibraryRow *row);   // satellite.variable.binary.string
signed long long int describe_1_6_5_9(LibraryRow *row);   // satellite.variable.binary.number
signed long long int describe_1_6_5_10(LibraryRow *row);   // satellite.variable.binary.binary
signed long long int describe_1_6_5_11(LibraryRow *row);   // satellite.variable.binary.hex
signed long long int describe_1_6_5_12(LibraryRow *row);   // satellite.variable.binary.clear
signed long long int describe_1_6_5_13(LibraryRow *row);   // satellite.variable.binary.add(x)
signed long long int describe_1_6_5_14(LibraryRow *row);   // satellite.variable.binary.reverse
signed long long int describe_1_6_6_1(LibraryRow *row);   // satellite.variable.bool.string
signed long long int describe_1_6_6_2(LibraryRow *row);   // satellite.variable.bool.add(x)
signed long long int describe_1_6_10_1(LibraryRow *row);   // satellite.variable.float.to_string
signed long long int describe_1_6_10_2(LibraryRow *row);   // satellite.variable.float.string
signed long long int describe_1_6_10_3(LibraryRow *row);   // satellite.variable.float.number
signed long long int describe_1_6_10_4(LibraryRow *row);   // satellite.variable.float.binary
signed long long int describe_1_6_10_5(LibraryRow *row);   // satellite.variable.float.hex
signed long long int describe_1_6_10_6(LibraryRow *row);   // satellite.variable.float.reverse
signed long long int describe_1_6_11_1(LibraryRow *row);   // satellite.variable.hex.to_number
signed long long int describe_1_6_11_2(LibraryRow *row);   // satellite.variable.hex.width
signed long long int describe_1_6_11_3(LibraryRow *row);   // satellite.variable.hex.to_string
signed long long int describe_1_6_11_4(LibraryRow *row);   // satellite.variable.hex.digits
signed long long int describe_1_6_11_5(LibraryRow *row);   // satellite.variable.hex.to_binary
signed long long int describe_1_6_11_7(LibraryRow *row);   // satellite.variable.hex.string
signed long long int describe_1_6_11_8(LibraryRow *row);   // satellite.variable.hex.number
signed long long int describe_1_6_11_9(LibraryRow *row);   // satellite.variable.hex.binary
signed long long int describe_1_6_11_10(LibraryRow *row);   // satellite.variable.hex.hex
signed long long int describe_1_6_11_11(LibraryRow *row);   // satellite.variable.hex.add(x)
signed long long int describe_1_6_11_12(LibraryRow *row);   // satellite.variable.hex.reverse
signed long long int describe_1_6_16_1(LibraryRow *row);   // satellite.variable.percentage.string
signed long long int describe_1_6_16_2(LibraryRow *row);   // satellite.variable.percentage.add(x)
signed long long int describe_1_6_19_1(LibraryRow *row);   // satellite.variable.color.transparency
signed long long int describe_1_6_19_2(LibraryRow *row);   // satellite.variable.color.string
signed long long int describe_1_6_19_3(LibraryRow *row);   // satellite.variable.color.number
signed long long int describe_1_6_19_4(LibraryRow *row);   // satellite.variable.color.binary
signed long long int describe_1_6_19_5(LibraryRow *row);   // satellite.variable.color.hex
signed long long int describe_1_6_20_1(LibraryRow *row);   // satellite.variable.fraction.numerator
signed long long int describe_1_6_20_2(LibraryRow *row);   // satellite.variable.fraction.denominator
signed long long int describe_1_6_20_3(LibraryRow *row);   // satellite.variable.fraction.string
signed long long int describe_1_6_20_4(LibraryRow *row);   // satellite.variable.fraction.reverse
signed long long int describe_1_6_4_1(LibraryRow *row);   // satellite.variable.number.shift_left(n)
signed long long int describe_1_6_4_2(LibraryRow *row);   // satellite.variable.number.max(a, b)
signed long long int describe_1_6_4_3(LibraryRow *row);   // satellite.variable.number.min(a, b)
signed long long int describe_1_6_4_4(LibraryRow *row);   // satellite.variable.number.abs(a)
signed long long int describe_1_6_4_5(LibraryRow *row);   // satellite.variable.number.clamp(a, low, high)
signed long long int describe_1_6_4_6(LibraryRow *row);   // satellite.variable.number.to_string
signed long long int describe_1_6_4_7(LibraryRow *row);   // satellite.variable.number.floor
signed long long int describe_1_6_4_8(LibraryRow *row);   // satellite.variable.number.ceil
signed long long int describe_1_6_4_9(LibraryRow *row);   // satellite.variable.number.round
signed long long int describe_1_6_4_10(LibraryRow *row);   // satellite.variable.number.power(a, b)
signed long long int describe_1_6_4_11(LibraryRow *row);   // satellite.variable.number.shift_right(n)
signed long long int describe_1_6_4_12(LibraryRow *row);   // satellite.variable.number.modulus(a, b)
signed long long int describe_1_6_4_13(LibraryRow *row);   // satellite.variable.number.truncate(a)
signed long long int describe_1_6_4_14(LibraryRow *row);   // satellite.variable.number.sqrt(a)
signed long long int describe_1_6_4_15(LibraryRow *row);   // satellite.variable.number.digits
signed long long int describe_1_6_4_17(LibraryRow *row);   // satellite.variable.number.string
signed long long int describe_1_6_4_18(LibraryRow *row);   // satellite.variable.number.number
signed long long int describe_1_6_4_19(LibraryRow *row);   // satellite.variable.number.binary
signed long long int describe_1_6_4_20(LibraryRow *row);   // satellite.variable.number.hex
signed long long int describe_1_6_4_21(LibraryRow *row);   // satellite.variable.number.add(x)
signed long long int describe_1_6_4_22(LibraryRow *row);   // satellite.variable.number.reverse
signed long long int describe_1_6_4_23(LibraryRow *row);   // satellite.variable.number.bytes
signed long long int describe_1_6_17_1(LibraryRow *row);   // satellite.variable.infinity.string
signed long long int describe_1_6_17_2(LibraryRow *row);   // satellite.variable.infinity.power_of(x)
signed long long int describe_1_6_17_3(LibraryRow *row);   // satellite.variable.infinity.resize(n)
signed long long int describe_1_6_17_4(LibraryRow *row);   // satellite.variable.infinity.nines
signed long long int describe_1_18_1(LibraryRow *row);   // satellite.directory.change(d)
signed long long int describe_1_18_4(LibraryRow *row);   // satellite.directory.list()
signed long long int describe_1_18_5(LibraryRow *row);   // satellite.directory.list(d)
signed long long int describe_1_18_6(LibraryRow *row);   // satellite.directory.system()
signed long long int describe_1_18_8(LibraryRow *row);   // satellite.directory.create(d)
signed long long int describe_1_25(LibraryRow *row);   // satellite.feedback
signed long long int describe_1_25_1(LibraryRow *row);   // satellite.feedback(x)

} // namespace built_in

struct BuiltInWord {
    const char *numbers;         // "1 6 1 5", as words/words.tsv writes it
    const char *name;            // "satellite.variable.string.substring(start, end)"
    DescribeFunction describe;   // fills the row, as the library's one export did
};

extern const BuiltInWord kBuiltInWords[];
extern const std::size_t kBuiltInWordCount;

} // namespace satellite004
