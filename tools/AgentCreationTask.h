/*
 Copyright (C) 2013 Erik Ogenvik

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef AGENTCREATIONTASK_H_
#define AGENTCREATIONTASK_H_

#include "common/ClientTask.h"

/**
 * A client task for creating a new agent in the game world.
 */
class AgentCreationTask: public ClientTask
{
    public:
        /**
         * Ctor.
         * @param account_id The id of the logged in account.
         * @param agent_type The type of agent to create.
         * @param agent_id A string into which the agent id
         * (once created) will be put.
         */
        AgentCreationTask(const std::string& account_id,
                const std::string& agent_type, std::string& agent_id);
        virtual ~AgentCreationTask();

        /// \brief Set up the task processing user arguments
        virtual void setup(const std::string & arg, OpVector &);
        /// \brief Handle an operation from the server
        virtual void operation(const Operation &, OpVector &);

    protected:

        /**
         * The id of the logged in account.
         */
        const std::string m_account_id;

        /**
         * The type of the agent.
         */
        const std::string m_agent_type;

        /**
         * Reference to an external string into which the id of the created
         * agent entity will be written.
         */
        std::string& m_agent_id;

        /**
         * Keeps track of the serial number of the sent op.
         */
        long int m_serial_no;
};

#endif /* AGENTCREATIONTASK_H_ */
