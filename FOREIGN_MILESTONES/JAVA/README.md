# Java 21 (LTS) → satellite, feature by feature

**The standard this folder is written against:** **Java SE 21**, the LTS release
(September 2023), per the Java Language Specification for that version.

**Written 2026-09-12 against knowledge current to May 2026.**

## The catalogue

| # | Java | satellite | answer |
|---|---|---|---|
| 1 | `System.out.println(x)` | `satellite.console.display(x)` | says it |
| 2 | `public static void main(String[] a)` | `satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)` | says it |
| 3 | `public class C` | `satellite.spacesuit C()` | says it |
| 4 | `private` / `public` members | `satellite.protected { }` / `satellite.public { }` | says it |
| 5 | `ArrayList<T>` | `satellite.container.list<T>` | says it |
| 6 | `HashMap<K,V>` | `satellite.container.map<K,V>` | says it |
| 7 | `String` | `satellite.variable.string` | says it |
| 8 | `Thread` / `.join()` | `satellite.thread.new` / `.start()` / `.join()` | says it |
| 9 | `new C()` | `C thing` — declaring constructs it | says it |
| 10 | garbage collection | refcounting; a cycle is never freed | says it, with a caveat |
| 11 | `&&` / `\|\|` / `!` | — | [M28](../SATELLITE/M28-logical-operators.md) |
| 12 | `break` / `continue` | — | [M27](../SATELLITE/M27-break-and-continue.md) |
| 13 | `switch` | — | [M30](../SATELLITE/M30-switch-and-match.md) |
| 14 | `try` / `catch` / `finally` | — | [M33](../SATELLITE/M33-catching-a-refusal.md) |
| 15 | generics `<T>` | — | [M31](../SATELLITE/M31-user-generics.md) |
| 16 | lambdas `->` / `Runnable` | — | [M32](../SATELLITE/M32-capsules-as-values.md) |
| 17 | interfaces | — | M31, then a decision |
| 18 | `extends` / `@Override` | partial — a suit holds a copy of its superclass | partial |
| 19 | `synchronized` | — | M40, and **nothing locks today** |
| 20 | annotations, reflection | — | never planned |

**The one to read first is 10.** Java readers expect a collector that handles
cycles. Satellite refcounts, and DESIGN §12 is open that a cycle is never freed
— so a self-referencing object leaks by design. That is acceptable in a program
that never releases anything and is a real leak in one that does.
