# 081 -- The run-time S240 (an answer was used, but none was handed back) names the capsule's header line, not the call whose answer was used

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** control flow and capsules  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

The run-time S240 (answer used, none handed back) names the capsule's header line, not the call whose answer was used.

## Program

```satellite
satellite.include(satellite)

satellite.capsule positive_only(satellite.variable.number n)
{
    satellite.statement.if(n > 0)
    {
        satellite.return(n)
    }
}

satellite.capsule satellite.main()
{
    satellite.variable.number a = positive_only(4)
    satellite.variable.number b = positive_only(-4)
    satellite.console.display(a + b)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none; c10.satl

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S240: CAPSULE_GAVE_NO_ANSWER
satl(run): positive_only's answer is used, and it ended without handing one back
-- the way it went reached no satellite.return(...) with a value

directory: c10.satl:3
syntax: satellite.capsule positive_only(satellite.variable.number n)
        /\

this capsule's answer was used, and the way it went reached no
satellite.return(...) -- so there was nothing to hand back.
machine code 53 capsule_gave_no_answer -- satl exits with this.

--------------------------------------------------------------------------------

exit 53
```

## What it should do

The report names c10.satl:14, 'satellite.variable.number b = positive_only(-4)', the call whose answer was used.

The check-time S240 for the same mistake names the call site (c10b.satl:10, 'syntax: satellite.variable.number x = nothing(3)'). The refusal is about an answer being used, which happens at the call. With two calls to one capsule, naming the header does not say which call failed.

## Variants

Check-time form (capsule with no valued return at all, 'satellite.variable.number x = nothing(3)'): names line 10, the call. The run-time form always names the header.

## Where it comes from

satellite/bytecode/program_walk.cpp:2073-2080 (run_site_here)

raise_at(capsule_gave_no_answer, ..., row, site.declared_at) uses the capsule's declaration position, and the frame does not carry the call site's position.

## Notes

- Why the skeptic kept it: Reproduced. When the capsule's answer is missing at run time, S240 names line 3, the header 'satellite.capsule positive_only(satellite.variable.number n)'. It does not name line 14, the call positive_only(-4) whose answer was used. Line 13 calls the same capsule and succeeds, so nothing in the report tells the person which call failed. The check-time S240 for the same kind of mistake names the call site (c10b.satl:10, 'satellite.variable.number x = nothing(3)'), so the two forms of one S-code disagree about which line to blame. The sentence itself ('positive_only's answer is used') is about the use, which happens at the call. Nothing in DESIGN.md, SATELLITE_ERROR.md or the known lists rules that the run-time S240 names the header. The already-known multi-line-statement item is about a different thing.
- Nearest known entry: none
- Merged: Single finding.
