/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   obsidian.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 01:57:12 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 03:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef OBSIDIAN_HPP
#define OBSIDIAN_HPP

#define OBSIDIAN_NAME "obsidian"
#define OBSIDIAN_VERSION "0.0.1-alpha"

/*
** Packed file layout:
**
**   [ stub image                      ]
**   [ stored payload (compressed then encrypted) ]
**   [ footer                          ]
*/

#define OBSD "OBSD"
#define OBSD_VERSION 3
#define OBSD_FOOTER_SIZE 40

/* Footer field offsets. */
#define OBSD_OFFSET_VERSION 4
#define OBSD_OFFSET_COMPRESSION 5
#define OBSD_OFFSET_ENCRYPTION 6
#define OBSD_OFFSET_NONCE_SIZE 7
#define OBSD_OFFSET_NONCE 8
#define OBSD_NONCE_MAX 12
#define OBSD_OFFSET_STORED_SIZE 20
#define OBSD_OFFSET_ORIGINAL_SIZE 28
#define OBSD_OFFSET_RESTORE 36

/*
** The stub reserves a key slot whose marker lets the packer locate it in the
** stub image. The 32 bytes that follow the marker hold the payload key.
*/

#define OBSIDIAN_KEY_SIZE 32
#define OBSIDIAN_KEY_MARKER "OBSIDIAN_KEYSLOT"
#define OBSIDIAN_KEY_MARKER_SIZE 16
#define OBSIDIAN_KEY_SLOT_SIZE 48

#endif
