#!/bin/bash

trap cleanup 1 2 3 6

cleanup()
{
  echo "Cleaning up temp files:"
  exit 1
}

### continue main script below
