// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contacts_picker;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.provider.ContactsContract;
import android.support.v7.widget.RecyclerView.Adapter;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.view.LayoutInflater;
import android.view.ViewGroup;

import org.chromium.chrome.R;

import java.io.ByteArrayInputStream;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

/**
 * A data adapter for the Contacts Picker.
 */
public class PickerAdapter extends Adapter<ViewHolder> {
    // The category view to use to show the contacts.
    private PickerCategoryView mCategoryView;

    // The content resolver to query data from.
    private ContentResolver mContentResolver;

    // A cursor containing the raw contacts data.
    private Cursor mContactsCursor;

    /**
     * Holds on to a {@link ContactView} that displays information about a contact.
     */
    public class ContactViewHolder extends ViewHolder {
        /**
         * The ContactViewHolder.
         * @param itemView The {@link ContactView} view for showing the contact details.
         */
        public ContactViewHolder(ContactView itemView) {
            super(itemView);
        }
    }

    private static final String[] PROJECTION = {
            ContactsContract.Contacts._ID, ContactsContract.Contacts.LOOKUP_KEY,
            ContactsContract.Contacts.DISPLAY_NAME_PRIMARY,
    };

    /**
     * The PickerAdapter constructor.
     * @param categoryView The category view to use to show the contacts.
     * @param contentResolver The content resolver to use to fetch the data.
     */
    public PickerAdapter(PickerCategoryView categoryView, ContentResolver contentResolver) {
        mCategoryView = categoryView;
        mContentResolver = contentResolver;
        mContactsCursor = contentResolver.query(ContactsContract.Contacts.CONTENT_URI, PROJECTION,
                null, null, ContactsContract.Contacts.DISPLAY_NAME_PRIMARY + " ASC");
    }

    /**
     * Sets the search query (filter) for the contact list. Filtering is by display name.
     * @param query The search term to use.
     */
    public void setSearchString(String query) {
        String searchString = "%" + query + "%";
        String[] selectionArgs = {searchString};
        mContactsCursor.close();

        mContactsCursor = mContentResolver.query(ContactsContract.Contacts.CONTENT_URI, PROJECTION,
                ContactsContract.Contacts.DISPLAY_NAME_PRIMARY + " LIKE ?", selectionArgs,
                ContactsContract.Contacts.DISPLAY_NAME_PRIMARY + " ASC");
        notifyDataSetChanged();
    }

    /**
     * Fetches all known contacts and their emails.
     * @return The contact list as a set.
     */
    public Set<ContactDetails> getAllContacts() {
        Set<ContactDetails> contacts = new HashSet<>();
        if (!mContactsCursor.moveToFirst()) return contacts;
        do {
            String id = mContactsCursor.getString(
                    mContactsCursor.getColumnIndex(ContactsContract.Contacts._ID));
            String name = mContactsCursor.getString(
                    mContactsCursor.getColumnIndex(ContactsContract.Contacts.DISPLAY_NAME_PRIMARY));
            contacts.add(new ContactDetails(id, name, getEmails(), getPhoneNumbers(), getPhoto()));
        } while (mContactsCursor.moveToNext());

        return contacts;
    }

    // RecyclerView.Adapter:

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        ContactView itemView = (ContactView) LayoutInflater.from(parent.getContext())
                                       .inflate(R.layout.contact_view, parent, false);
        itemView.setCategoryView(mCategoryView);
        return new ContactViewHolder(itemView);
    }

    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {
        String id = "";
        String name = "";
        if (mContactsCursor.moveToPosition(position)) {
            id = mContactsCursor.getString(
                    mContactsCursor.getColumnIndex(ContactsContract.Contacts._ID));
            name = mContactsCursor.getString(
                    mContactsCursor.getColumnIndex(ContactsContract.Contacts.DISPLAY_NAME_PRIMARY));
        }

        ((ContactView) holder.itemView)
                .initialize(
                        new ContactDetails(id, name, getEmails(), getPhoneNumbers(), getPhoto()));
    }

    private ArrayList<String> getEmails() {
        // Look up all associated emails for this contact.
        String id = mContactsCursor.getString(
                mContactsCursor.getColumnIndex(ContactsContract.Contacts._ID));
        Cursor emailCursor =
                mContentResolver.query(ContactsContract.CommonDataKinds.Email.CONTENT_URI, null,
                        ContactsContract.CommonDataKinds.Email.CONTACT_ID + " = " + id, null,
                        ContactsContract.CommonDataKinds.Email.DATA + " ASC");
        ArrayList<String> emails = new ArrayList<String>();
        while (emailCursor.moveToNext()) {
            emails.add(emailCursor.getString(
                    emailCursor.getColumnIndex(ContactsContract.CommonDataKinds.Email.DATA)));
        }
        emailCursor.close();
        return emails;
    }

    private ArrayList<String> getPhoneNumbers() {
        // Look up all associated phone numbers for this contact.
        String id = mContactsCursor.getString(
                mContactsCursor.getColumnIndex(ContactsContract.Contacts._ID));
        Cursor phoneNumberCursor =
                mContentResolver.query(ContactsContract.CommonDataKinds.Phone.CONTENT_URI, null,
                        ContactsContract.CommonDataKinds.Phone.CONTACT_ID + " = " + id, null,
                        ContactsContract.CommonDataKinds.Phone.NUMBER + " ASC");
        ArrayList<String> phoneNumbers = new ArrayList<String>();
        while (phoneNumberCursor.moveToNext()) {
            phoneNumbers.add(phoneNumberCursor.getString(
                    phoneNumberCursor.getColumnIndex(ContactsContract.CommonDataKinds.Email.DATA)));
        }
        phoneNumberCursor.close();
        return phoneNumbers;
    }

    private Bitmap getPhoto() {
        String id = mContactsCursor.getString(
                mContactsCursor.getColumnIndex(ContactsContract.Contacts._ID));
        Uri contactUri = ContentUris.withAppendedId(
                ContactsContract.Contacts.CONTENT_URI, Long.parseLong(id));
        Uri photoUri =
                Uri.withAppendedPath(contactUri, ContactsContract.Contacts.Photo.CONTENT_DIRECTORY);
        Cursor cursor = mContentResolver.query(
                photoUri, new String[] {ContactsContract.Contacts.Photo.PHOTO}, null, null, null);
        if (cursor == null) return null;
        try {
            if (cursor.moveToFirst()) {
                byte[] data = cursor.getBlob(0);
                if (data != null) {
                    return BitmapFactory.decodeStream(new ByteArrayInputStream(data));
                }
            }
        } finally {
            cursor.close();
        }
        return null;
    }

    @Override
    public int getItemCount() {
        return mContactsCursor.getCount();
    }
}
