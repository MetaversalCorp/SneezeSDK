#!/bin/sh
# Recompute the collection version from the per-SDK VERSION files.
#
# Each language SDK declares its own version in a VERSION file at its root, in
# MAJOR.MINOR.PATCH form. The collection shares the MAJOR.MINOR line and takes
# the highest PATCH found across the SDKs, so the umbrella never lags behind its
# newest member:
#
#    SneezeSDK 0.1.Y   where Y = MAX (A, B, C, D, E)
#
# An SDK whose submodule is not checked out is skipped, so this stays usable in
# a shallow clone. Usage:
#
#    version-sync.sh           rewrite the root VERSION file when it is stale
#    version-sync.sh --check   report staleness without writing (exit 1 if stale)

set -e

SDKS='AS C Cpp CS Rust'

Fail ()
{
   printf 'version-sync: %s\n' "$1" >&2
   exit 1
}

Version_Read ()
{
   tr -d ' \011\015\012' < "$1"
}

# A toolchain manifest that carries its own copy of the version is a mirror, not
# a second source of truth. Report drift rather than silently picking a winner.
Drift_Report ()
{
   sSdk="$1"
   sManifest="$2"
   sDeclared="$3"
   sVersion="$4"

   if [ -n "$sDeclared" ]  &&  [ "$sDeclared" != "$sVersion" ]
   then
      printf 'version-sync: warning: %s declares %s but %s/VERSION declares %s\n' "$sManifest" "$sDeclared" "$sSdk" "$sVersion" >&2
   fi
}

bCheckOnly=0

if [ "$1" = '--check' ]
then
   bCheckOnly=1
elif [ -n "$1" ]
then
   Fail "unknown argument '$1'"
fi

sRoot=$(git rev-parse --show-toplevel 2>/dev/null) || sRoot=''

if [ -z "$sRoot" ]
then
   sRoot=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
fi

sPrefix=''
nMaxPatch=-1
sFound=''
sMissing=''

for sSdk in $SDKS
do
   sFile="$sRoot/$sSdk/VERSION"

   if [ -f "$sFile" ]
   then
      sVersion=$(Version_Read "$sFile")

      printf '%s' "$sVersion" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+$'  ||  Fail "$sSdk/VERSION is not MAJOR.MINOR.PATCH: '$sVersion'"

      sThisPrefix=$(printf '%s' "$sVersion" | cut -d. -f1,2)
      nThisPatch=$(printf '%s' "$sVersion" | cut -d. -f3)

      if [ -z "$sPrefix" ]
      then
         sPrefix="$sThisPrefix"
      elif [ "$sThisPrefix" != "$sPrefix" ]
      then
         Fail "$sSdk declares $sVersion, off the $sPrefix.x line the other SDKs share"
      fi

      if [ "$nThisPatch" -gt "$nMaxPatch" ]
      then
         nMaxPatch="$nThisPatch"
      fi

      sFound="$sFound $sSdk=$sVersion"
   else
      sMissing="$sMissing $sSdk"
   fi
done

[ -n "$sPrefix" ]  ||  Fail 'no SDK VERSION files found - are the submodules checked out?'

if [ -f "$sRoot/Rust/Cargo.toml" ]
then
   Drift_Report 'Rust' 'Rust/Cargo.toml' "$(sed -n 's/^version *= *"\([^"]*\)".*/\1/p' "$sRoot/Rust/Cargo.toml" | head -1)" "$(Version_Read "$sRoot/Rust/VERSION")"
fi

if [ -f "$sRoot/AS/package.json" ]
then
   Drift_Report 'AS' 'AS/package.json' "$(sed -n 's/.*"version" *: *"\([^"]*\)".*/\1/p' "$sRoot/AS/package.json" | head -1)" "$(Version_Read "$sRoot/AS/VERSION")"
fi

sTarget="$sPrefix.$nMaxPatch"
sCurrent=''

if [ -f "$sRoot/VERSION" ]
then
   sCurrent=$(Version_Read "$sRoot/VERSION")
fi

if [ -n "$sMissing" ]
then
   printf 'version-sync: skipped (not checked out):%s\n' "$sMissing" >&2
fi

nStatus=0

if [ "$sCurrent" = "$sTarget" ]
then
   printf 'version-sync: VERSION is %s (max of:%s)\n' "$sTarget" "$sFound"
else
   if [ "$bCheckOnly" -eq 1 ]
   then
      printf 'version-sync: VERSION is %s but should be %s (max of:%s)\n' "$sCurrent" "$sTarget" "$sFound" >&2
      nStatus=1
   else
      printf '%s\n' "$sTarget" > "$sRoot/VERSION"
      printf 'version-sync: VERSION %s -> %s (max of:%s)\n' "$sCurrent" "$sTarget" "$sFound"
   fi
fi

exit $nStatus
