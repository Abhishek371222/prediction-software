#!/usr/bin/env node
import 'dotenv/config';
import { syncVersionsJson } from './lib/sync.mjs';

await syncVersionsJson();
