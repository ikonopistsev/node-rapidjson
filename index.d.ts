/// <reference types="node" />

declare class RapidJSON {
  constructor(RAMUsage?: number);

  stringify(payload: any): string;
  parse(payload: string): any;
  parseBigInt(payload: string): any;
  forceBigInt(arr: Array<string>): void;
}

export = RapidJSON;
