// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * An Observable with at most one activation at a time, with mutators that set or reset the
 * activation data.
 *
 * Mutator methods on this class are sequenced: if calling one causes an Observer to call another
 * sequenced mutator method synchronously, the nested call will be deferred until the outer call is
 * finished. This ensures that all subscribed Observers are notified of all state changes.
 *
 * @param <T> The type of the activation data.
 */
public class Controller<T> extends Observable<T> {
    private final Sequencer mSequencer = new Sequencer();
    private final List<Observer<? super T>> mObservers = new ArrayList<>();
    private final Map<Observer<? super T>, Scope> mScopeMap = new HashMap<>();
    private T mData;

    @Override
    public Subscription subscribe(Observer<? super T> observer) {
        mSequencer.sequence(() -> {
            mObservers.add(observer);
            if (mData != null) notifyEnter(observer);
        });
        return () -> mSequencer.sequence(() -> {
            if (mData != null) notifyExit(observer);
            mObservers.remove(observer);
        });
    }

    /**
     * Activates this Controller, opening scopes with the given data for all observers.
     *
     * If this is already activated, all observers' opened scopes will first be closed, as if
     * reset() was called, before scopes for the new data are opened. However, if the new data is
     * equal to the old data, this is a no-op.
     */
    public void set(T data) {
        mSequencer.sequence(() -> {
            // set(null) is equivalent to reset().
            if (data == null) {
                resetInternal();
                return;
            }
            // If this Controller was already set(), call reset() so observing Scopes can clean up.
            if (mData != null) {
                // However, if this Controller was already set() with this data, no-op.
                if (mData.equals(data)) return;
                resetInternal();
            }

            mData = data;
            for (int i = 0; i < mObservers.size(); i++) {
                notifyEnter(mObservers.get(i));
            }
        });
    }

    /**
     * Deactivates this Controller, closing the opened scopes for all observers.
     *
     * If this Controller is already deactivated, this is a no-op.
     */
    public void reset() {
        mSequencer.sequence(() -> resetInternal());
    }

    private void resetInternal() {
        assert mSequencer.inSequence();
        if (mData == null) return;
        mData = null;
        for (int i = mObservers.size() - 1; i >= 0; i--) {
            notifyExit(mObservers.get(i));
        }
    }

    private void notifyEnter(Observer<? super T> observer) {
        assert mSequencer.inSequence();
        Scope scope = observer.open(mData);
        assert scope != null;
        mScopeMap.put(observer, scope);
    }

    private void notifyExit(Observer<? super T> observer) {
        assert mSequencer.inSequence();
        Scope scope = mScopeMap.get(observer);
        assert scope != null;
        mScopeMap.remove(observer);
        scope.close();
    }

    // TODO(sanfin): make this its own public class and add tests.
    private static class Sequencer {
        private boolean mIsRunning;
        private final ArrayDeque<Runnable> mMessageQueue = new ArrayDeque<>();

        /**
         * Runs the task synchronously, or, if a sequence()d task is already running, posts the task
         * to a queue, whose items will be run synchronously when the current task is finished.
         */
        public void sequence(Runnable impl) {
            if (mIsRunning) {
                mMessageQueue.add(() -> sequence(impl));
                return;
            }
            mIsRunning = true;
            impl.run();
            mIsRunning = false;
            while (!mMessageQueue.isEmpty()) {
                mMessageQueue.removeFirst().run();
            }
        }

        public boolean inSequence() {
            return mIsRunning;
        }
    }
}
