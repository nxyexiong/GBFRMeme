// Try different 32-bit hash variants until we find one that matches the
// live CharaList keys. The 22 unknown hashes from your save are dumped in
// from a file (one hex per line). Candidate strings are generated as
// pl0000..pl9999 (and a few common forms).
//
// Usage from PowerShell:
//   $exe = ".\build\src\app\Release\GBFRMeme.exe"
//   $live = & $exe -c characters | Select-String '#' | ForEach-Object {
//       ($_ -split '#')[1].Trim()
//   }
//   $live | Set-Content build\tmp\live_hashes.txt
//   .\build\src\app\Release\GBFRMeme.exe -c debug hash-find build\tmp\live_hashes.txt
//
// This file *does not* exist yet; replaced by the dispatch.

#pragma once
