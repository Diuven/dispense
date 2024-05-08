"use client";

import React, { useState, useCallback, useEffect } from "react";
import useWebSocket, { ReadyState } from "react-use-websocket";
import { NodeHandler } from "./computing";

export default function Home() {
  const __SERVER_URL = "ws://10.29.230.222:9000";
  const [socketUrl, setSocketUrl] = useState(__SERVER_URL);
  // random between 1 ~ 9999
  const [nodeId, setNodeId] = useState(1);
  const [nodeHandler, setNodeHandler] = useState<NodeHandler>();
  const { sendMessage, lastMessage, readyState } = useWebSocket(socketUrl);

  useEffect(() => {
    const newNodeId = Math.floor(Math.random() * 9999) + 1;
    setNodeId(newNodeId);
  }, []);

  const stopHandler = useCallback(() => {
    console.log("NodeHandler stopped. Exiting...");
    disconnectFromServer();
  }, []);

  useEffect(() => {
    console.log("nodeId set: ", nodeId);
    setNodeHandler(new NodeHandler(nodeId, stopHandler));
  }, [nodeId]);

  const messageHandler = useCallback(
    async (gotMessage: MessageEvent<any>) => {
      if (!nodeHandler || !sendMessage) return;
      const message = gotMessage.data;
      // console.log("got message: ", message);
      const response = await nodeHandler.messageHandler(message);
      if (!response) return;
      // console.log("sending response: ", response);
      sendMessage(response);
    },
    [nodeHandler, sendMessage]
  );

  const connectToServer = useCallback(() => {
    if (!nodeHandler || !sendMessage || !readyState) return;

    const message = nodeHandler.makeEnterMessage();
    if (!message) return;
    console.log("connected to server. Sending enter message: ", message);
    sendMessage(message);
  }, [nodeHandler, sendMessage, readyState]);

  const disconnectFromServer = useCallback(() => {
    if (!nodeHandler || !sendMessage || !readyState) return;

    const message = nodeHandler.makeCloseMessage();
    if (!message) return;
    console.log("Sending exit message: ", message);
    sendMessage(message);
  }, [nodeHandler, sendMessage, readyState]);

  useEffect(() => {
    if (!lastMessage) return;
    messageHandler(lastMessage);
  }, [lastMessage, messageHandler]);

  return (
    <main className="flex min-h-screen flex-col items-center justify-between p-24">
      <h1 className="text-4xl font-bold items-center justify-center">
        6.5610 Project
        <br />
        Secure Distributed Matrix Multiplication
      </h1>
      <br />

      <button
        onClick={connectToServer}
        disabled={readyState !== ReadyState.OPEN}
        className="flex items-center gap-2 p-4 border-l border-gray-300 bg-gradient-to-b from-zinc-200 pb-6 pt-8 backdrop-blur-2xl dark:border-neutral-800 dark:bg-zinc-800/30 dark:from-inherit lg:static lg:rounded-xl lg:border lg:bg-gray-200 lg:p-4 lg:dark:bg-zinc-800/30"
      >
        Click to enter to the pool
      </button>
      {/* <button
        onClick={disconnectFromServer}
        disabled={readyState !== ReadyState.OPEN}
        className="fixed right-0 top-0 flex items-center gap-2 p-4 border-l border-gray-300 bg-gradient-to-b from-zinc-200 pb-6 pt-8 backdrop-blur-2xl dark:border-neutral-800 dark:bg-zinc-800/30 dark:from-inherit lg:static lg:rounded-xl lg:border lg:bg-gray-200 lg:p-4 lg:dark:bg-zinc-800/30"
      >
        Click Me to exit from the pool
      </button> */}

      <span className="items-center">Your node ID: {nodeId}</span>
      <span>Your computated count: {nodeHandler?.computed_count}</span>

      <span className="items-center">
        The connection is currently{" "}
        {Object.keys(ReadyState).find((key) => ReadyState[key] === readyState)}{" "}
      </span>
      {lastMessage ? (
        <span className="items-center">
          Last operation: {lastMessage.data.split(";;")[1]}
        </span>
      ) : null}

      <br />

      <div className="mb-32 grid text-center lg:mb-0 lg:w-full lg:max-w-5xl lg:grid-cols-4 lg:text-left">
        <a
          href="https://github.com/Diuven/dispense"
          className="group rounded-lg border border-transparent px-5 py-4 transition-colors hover:border-gray-300 hover:bg-gray-100 hover:dark:border-neutral-700 hover:dark:bg-neutral-800/30"
          target="_blank"
          rel="noopener noreferrer"
        >
          <h2 className="mb-3 text-2xl font-semibold">
            Repository{" "}
            <span className="inline-block transition-transform group-hover:translate-x-1 motion-reduce:transform-none">
              -&gt;
            </span>
          </h2>
          <p className="m-0 max-w-[30ch] text-sm opacity-50">
            Check out the code!
          </p>
        </a>
      </div>
    </main>
  );
}
